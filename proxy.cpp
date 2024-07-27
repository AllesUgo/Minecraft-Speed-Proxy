#include "proxy.h"
#include "datapackage.h"
#include <iostream>
#include <mutex>

Proxy::Proxy(bool is_ipv6_local, const std::string& local_address, std::uint16_t local_port, bool is_ipv6_remote, const std::string& remote_server_addr, std::uint16_t remote_server_port)
	: local_server(local_port,local_address,is_ipv6_local), remote_server_addr(remote_server_addr), remote_server_port(remote_server_port), is_ipv6_remote(is_ipv6_remote)
{

}

void Proxy::Start()
{
	this->thread_pool.Run([this]() {
		try
		{
			while (true) {
				auto connection = this->local_server.Accept();
				this->thread_pool.Run([this, connection]() {
					try {
						//加入连接池
						std::unique_lock<std::shared_mutex> lock(this->global_mutex);
						this->connections.push_back(connection);
						lock.unlock();
						//调用连接回调
						this->on_connected(connection);
						int connection_status = 0;//未握手
						//必须先握手
						HandshakeDataPack handshake_data_pack;
						RbsLib::Network::TCP::TCPStream stream(connection);
						handshake_data_pack.ParseFromInputStream(stream);
						connection_status = handshake_data_pack.next_state.Value();
						while (true) {
							switch (connection_status) {
							case 1: {
								RbsLib::Buffer buffer = DataPack::ReadFullData(stream);
								switch (NoCompressionDataPack::GetID(buffer))
								{
								case 0: {
									//状态请求，直接响应
									StatusResponseDataPack status_response_data_pack;
									status_response_data_pack.json_response = RbsLib::DataType::String("{\
										\"version\": {\
										\"name\": \"1.8.7\",\
											\"protocol\" : 47\
									},\
										\"players\" : {\
										\"max\": 100,\
											\"online\" : 5,\
											\"sample\" : [\
										{\
											\"name\": \"thinkofdeath\",\
												\"id\" : \"4566e69f-c907-48ee-8d71-d7ba5aa00d20\"\
										}\
											]\
									},\
										\"description\": {\
										\"text\": \"Hello world\"\
									},\
										\"favicon\" : \"data:image/png;base64,<data>\"\
								}");
									stream.Write(status_response_data_pack.ToBuffer());
									break;
								}
								case 1: {
									//ping,响应pong
									PingDataPack ping_data_pack;
									RbsLib::Streams::BufferInputStream bis(buffer);
									ping_data_pack.ParseFromInputStream(bis);
									PingDataPack pong_data_pack;
									pong_data_pack.payload = ping_data_pack.payload;
									stream.Write(pong_data_pack.ToBuffer());
									break;
								}
								default:
									throw ProxyException("Invalid data pack id.");
								}
								break;
							}
							case 2: {
								//登录请求，接收登录请求
								StartLoginDataPack start_login_data_pack;
								start_login_data_pack.ParseFromInputStream(stream);
								//调用登录回调
								try {
									if (start_login_data_pack.have_uuid) 
										this->on_login(connection, start_login_data_pack.user_name, start_login_data_pack.GetUUID());
									else 
										this->on_login(connection, start_login_data_pack.user_name, std::string());
								}
								catch (const CallbackException& e) {
									//登录失败
									LoginFailureDataPack login_failed_data_pack(e.what());
									stream.Write(login_failed_data_pack.ToBuffer());
									throw ProxyException(std::string("Callback disable user login: ")+e.what());
								}
								//连接远程服务器
								auto remote_server = is_ipv6_remote ? RbsLib::Network::TCP::TCPClient::Connect6(remote_server_addr, remote_server_port) : RbsLib::Network::TCP::TCPClient::Connect(remote_server_addr, remote_server_port);
								int flag = 1;
								remote_server.SetSocketOption(IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
								flag = 1;
								connection.SetSocketOption(IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
								auto user_ptr = std::make_shared<User>(connection, remote_server);
								user_ptr->username = start_login_data_pack.user_name;
								user_ptr->uuid = start_login_data_pack.GetUUID();
								user_ptr->ip = connection.GetAddress();
								user_ptr->connect_time = std::time(nullptr);
								//加入用户池
								lock.lock();
								this->users[user_ptr->username] = user_ptr;
								lock.unlock();
								try {
									//发送握手申请
									remote_server.Send(handshake_data_pack.ToBuffer());
									//发送登录请求
									remote_server.Send(start_login_data_pack.ToBuffer());
									//进入代理
									RbsLib::Buffer buffer(1024);
									//将该用户记录
									
									this->thread_pool.Run([this, connection, remote_server,user_ptr]() {
										std::unique_lock<std::shared_mutex> lock(this->global_mutex);
										this->connections.push_back(connection);
										lock.unlock();
										try
										{
											RbsLib::Buffer buffer(1024);
											while (true) {
												remote_server.Recv(buffer);
												connection.Send(buffer);
												user_ptr->upload_bytes+= buffer.GetLength();
											}
										}
										catch (const std::exception& e)
										{
											connection.Disable();
											remote_server.Disable();
											lock.lock();
											this->connections.remove(remote_server);
										}
										});
									
									RbsLib::Buffer b(1024);
									while (true) {
										connection.Recv(b);
										remote_server.Send(b);
										user_ptr->upload_bytes += b.GetLength();
									}
								}
								catch (...) {
									remote_server.Disable();
									this->users.erase(start_login_data_pack.user_name);
									UserInfo user_info;
									user_info.username = user_ptr->username;
									user_info.uuid = user_ptr->uuid;
									user_info.ip = user_ptr->ip;
									user_info.upload_bytes = user_ptr->upload_bytes;
									user_info.connect_time = user_ptr->connect_time;
									this->on_logout(connection,user_info);
									throw;
								}
							}
								  break;
							default:
								throw ProxyException("Invalid connection status.");
							}
						}
					}
					catch (const std::exception& e) {
						connection.Disable();
						std::unique_lock<std::shared_mutex> lock(this->global_mutex);
						this->connections.remove(connection);
						try {
							this->on_disconnect(connection);
						}
						catch (...) {
						}
					}
					});
			}
		}
		catch (const std::exception& ex) {
		}
		});

}

void Proxy::KickByUsername(const std::string& username) {
	std::shared_lock<std::shared_mutex> lock(this->global_mutex);
	auto iterator = this->users.find(username);
	if (iterator != this->users.end()) {
		iterator->second->client.Disable();
		iterator->second->server.Disable();
	}
	else throw ProxyException("User not found");
}

void Proxy::KickByUUID(const std::string& uuid)
{
	std::shared_lock<std::shared_mutex> lock(this->global_mutex);
	for (auto& user : this->users) {
		if (user.second->uuid == uuid) {
			user.second->client.Disable();
			user.second->server.Disable();
			return;
		}
	}
	throw ProxyException("User not found");
}

auto Proxy::GetUsersInfo() -> std::list<UserInfo>
{
	std::shared_lock<std::shared_mutex> lock(this->global_mutex);
	std::list<UserInfo> users_info;
	for (auto& user : this->users) {
		UserInfo user_info;
		user_info.username = user.second->username;
		user_info.uuid = user.second->uuid;
		user_info.ip = user.second->ip;
		user_info.upload_bytes = user.second->upload_bytes;
		user_info.connect_time = user.second->connect_time;
		users_info.push_back(user_info);
	}
	return users_info;
}

Proxy::~Proxy() noexcept
{
	this->local_server.ForceClose();
	std::unique_lock<std::shared_mutex> lock(this->global_mutex);
	for (auto& connection : this->connections) {
		connection.Disable();
	}
	this->connections.clear();
	lock.unlock();
}

ProxyException::ProxyException(const std::string& message) noexcept
	: message(message)
{
}

const char* ProxyException::what() const noexcept
{
	return this->message.c_str();
}

User::User(const RbsLib::Network::TCP::TCPConnection& client, const RbsLib::Network::TCP::TCPConnection& server)
	: client(client), server(server)
{
}

Proxy::CallbackException::CallbackException(const std::string& message) noexcept
	: message(message)
{
}

const char* Proxy::CallbackException::what() const noexcept
{
	return this->message.c_str();
}
