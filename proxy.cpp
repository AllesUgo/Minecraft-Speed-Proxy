#include "datapackage.h"
#include "proxy.h"
#include <chrono>
#include <iostream>
#include <mutex>

Proxy::Proxy(const std::string& local_address, std::uint16_t local_port, const std::string& remote_server_addr, std::uint16_t remote_server_port)
	: local_server(local_port,local_address), remote_server_addr(remote_server_addr), remote_server_port(remote_server_port)
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
						std::string remote_server_suffix;
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
									std::shared_lock<std::shared_mutex> lock(this->motd_mutex);
									auto motd = this->motd;
									lock = std::shared_lock<std::shared_mutex>(this->global_mutex);
									motd.SetOnlinePlayerNumber(this->users.size());
									motd.SetSampleUsers(this->GetUsersInfo());
									motd.SetVersion("", handshake_data_pack.protocol_version.Value());
									this->max_player!=-1?motd.SetPlayerMaxNumber(this->max_player):motd.SetPlayerMaxNumber(9999999);
									lock.unlock();
									status_response_data_pack.json_response = RbsLib::DataType::String(motd.ToString());
									stream.Write(status_response_data_pack.ToBuffer());
									//debug motd
									//std::cout << motd.motd_json.ToFormattedString() << std::endl;
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
								//检查是否达到最大玩家数
								std::shared_lock<std::shared_mutex> check_max_player(this->global_mutex);
								if (this->max_player != -1 && this->users.size() >= this->max_player) {
									LoginFailureDataPack login_failed_data_pack("Server is full.");
									stream.Write(login_failed_data_pack.ToBuffer());
									throw ProxyException("Server is full.");
								}
								check_max_player.unlock();
								StartLoginDataPack start_login_data_pack;
								auto login_start_row_packet = DataPack::ReadFullData(stream);
								RbsLib::Streams::BufferInputStream bis(login_start_row_packet);
								start_login_data_pack.ParseFromInputStream(bis);
								//检查是否是FML登录
								if (handshake_data_pack.server_address.size()!=std::strlen(handshake_data_pack.server_address.c_str()))
								{
									remote_server_suffix = handshake_data_pack.server_address.substr(std::strlen(handshake_data_pack.server_address.c_str()));
								}
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
									throw ProxyException(std::string("Callback disable user login: ") + e.what());
								}
								//连接远程服务器
								std::string remote_server_addr_real;
								std::uint32_t remote_server_port_real;
								std::shared_lock<std::shared_mutex> share_lock(this->global_mutex);
                                if (auto usr = this->user_proxy_map.find(start_login_data_pack.user_name); usr != this->user_proxy_map.end())
                                {
									remote_server_addr_real = usr->second.first;
									remote_server_port_real = usr->second.second;
                                }
								else
								{
									remote_server_addr_real = this->remote_server_addr;
									remote_server_port_real = this->remote_server_port;
								}
								share_lock.unlock();
								auto remote_server = RbsLib::Network::TCP::TCPClient::Connect(remote_server_addr_real, remote_server_port_real);
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
								if (this->users.find(start_login_data_pack.user_name) != this->users.end()) 
								{
									lock.unlock();
									LoginFailureDataPack login_failed_data_pack(start_login_data_pack.user_name+" already online");
									stream.Write(login_failed_data_pack.ToBuffer());
									throw ProxyException("User already online: " + start_login_data_pack.user_name);
								}
								this->users[user_ptr->username] = user_ptr;
								lock.unlock();
								try {
									//发送握手申请
									handshake_data_pack.server_address = RbsLib::DataType::String(this->remote_server_addr+remote_server_suffix);
									

									remote_server.Send(handshake_data_pack.ToBuffer());
									//发送登录请求
									remote_server.Send(login_start_row_packet);
									//进入代理
									RbsLib::Buffer buffer(1024);
									//将该用户记录
									
									this->thread_pool.Run([this, connection, remote_server,user_ptr]() {
										std::unique_lock<std::shared_mutex> lock(this->global_mutex);
										this->connections.push_back(connection);
										lock.unlock();
										try
										{
											std::unique_ptr<char[]> buffer = std::make_unique<char[]>(1024);
											int size;
											while (true) {
												size = remote_server.Recv(buffer.get(), 1024, 0);
												if (size<=0) throw ProxyException("Remote server disconnected or error.");
												if (connection.Send(buffer.get(),size,0)<=0)
													throw ProxyException("Client disconnected or error.");
												user_ptr->upload_bytes += size;
											}
										}
										catch (const std::exception& e)
										{
											exception_handle(e);
											connection.Disable();
											remote_server.Disable();
											lock.lock();
											this->connections.remove(remote_server);
										}
										});
									
									std::unique_ptr<char[]> b = std::make_unique<char[]>(1024);
									int size;
									while (true) {
										size = connection.Recv(b.get(),1024,0);
										if (size<=0) throw ProxyException("Client disconnected or error.");
										if (remote_server.Send(b.get(),size,0)<=0)
											throw ProxyException("Remote server disconnected or error.");
										user_ptr->upload_bytes += size;
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
						exception_handle(e);
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

void Proxy::SetMotd(const std::string& motd)
{
	std::unique_lock<std::shared_mutex> lock(this->motd_mutex);
	neb::CJsonObject motd_json;
	if (motd_json.Parse(motd)==false) throw ProxyException("Invalid motd json.");
	this->motd.motd_json = motd_json;
}

void Proxy::SetMaxPlayer(int n)
{
	if (n<-1) throw ProxyException("Can not set maxplayer less than -1.");
	std::unique_lock<std::shared_mutex> lock(this->global_mutex);
	this->max_player = n;
}

int Proxy::GetMaxPlayer(void)
{
	std::shared_lock<std::shared_mutex> lock(this->global_mutex);
	return this->max_player;
}

void Proxy::SetUserProxy(const std::string& username, const std::string& proxy_address, std::uint16_t proxy_port)
{
	std::unique_lock<std::shared_mutex> lock(this->global_mutex);
	this->user_proxy_map[username] = std::make_pair(proxy_address, proxy_port);
}

auto Proxy::GetUserProxyMap() const -> std::map<std::string, std::pair<std::string, std::uint16_t>>
{
	std::shared_lock<std::shared_mutex> lock(this->global_mutex);
	return this->user_proxy_map;
}

void Proxy::DeleteUserProxy(const std::string& username)
{
	std::unique_lock<std::shared_mutex> lock(this->global_mutex);
	auto it = this->user_proxy_map.find(username);
	if (it != this->user_proxy_map.end()) {
		this->user_proxy_map.erase(it);
	} else {
		throw ProxyException("User proxy not found.");
	}
}

void Proxy::ClearUserProxy()
{
	std::unique_lock<std::shared_mutex> lock(this->global_mutex);
	this->user_proxy_map.clear();
}

auto Proxy::GetDefaultProxy() -> std::pair<std::string, std::uint16_t>
{
	return std::pair<std::string, std::uint16_t>(this->remote_server_addr, this->remote_server_port);
}

auto Proxy::PingTest() const -> std::uint64_t
{
	try
	{
		auto remote_server = RbsLib::Network::TCP::TCPClient::Connect(remote_server_addr, remote_server_port);
		int flag = 1;
		remote_server.SetSocketOption(IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
		HandshakeDataPack handshake_data_pack;
		handshake_data_pack.id = 0;
		handshake_data_pack.server_address = RbsLib::DataType::String(remote_server_addr);
		handshake_data_pack.server_port = remote_server_port;
		handshake_data_pack.next_state = 1;
		handshake_data_pack.protocol_version = 754;
		remote_server.Send(handshake_data_pack.ToBuffer());
		RbsLib::Network::TCP::TCPStream stream(remote_server);
		//发送状态请求
		StatusRequestDataPack status_request_data_pack;
		remote_server.Send(status_request_data_pack.ToBuffer());
		//接收状态并丢弃
		DataPack::Data(stream);
		//ping
		PingDataPack ping_data_pack,pong_data_pack;
		ping_data_pack.payload = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		auto start = std::chrono::system_clock::now();
		remote_server.Send(ping_data_pack.ToBuffer());
		pong_data_pack.ParseFromInputStream(stream);
		auto end = std::chrono::system_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		if (pong_data_pack.payload != ping_data_pack.payload) throw ProxyException("Ping test failed.");
		return duration.count();
	}
	catch (const std::exception& e)
	{
		throw ProxyException(std::string("Ping test failed: ") + e.what());
	}
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

void Motd::SetVersion(const std::string& version_name, int protocol)
{
	this->motd_json.AddEmptySubObject("version");
	this->motd_json["version"].Add("name", version_name);
	this->motd_json["version"].Add("protocol", protocol);
}

void Motd::SetPlayerMaxNumber(int n)
{
	this->motd_json.AddEmptySubObject("players");
	this->motd_json["players"].Add("max", n);
}

void Motd::SetOnlinePlayerNumber(int n)
{
	this->motd_json.AddEmptySubObject("players");
	this->motd_json["players"].Add("online", n);
}

void Motd::SetSampleUsers(std::list<UserInfo> const& users)
{
	this->motd_json.AddEmptySubObject("players");
	if (this->motd_json["players"].KeyExist("sample") == true)
		return;//如果已经存在sample则不添加
	this->motd_json["players"].AddEmptySubArray("sample");
	if (this->motd_json["players"]["sample"].GetArraySize()==0) 
	{
		for (auto& user : users) 
		{
			if (user.uuid.empty()) continue;
			neb::CJsonObject user_json;
			user_json.Add("name", user.username);
			user_json.Add("id", user.uuid);
			this->motd_json["players"]["sample"].Add(user_json);
		}
	}
}

auto Motd::ToString() -> std::string
{
	return this->motd_json.ToString();
}

auto Motd::LoadMotdFromFile(const std::string& path) -> std::string
{
	if (path.empty())
		return "{\"description\": {\"text\": \"Minecraft Speed Proxy\"}}";
	try
	{
		RbsLib::Storage::FileIO::File file_io(path);
		return file_io.Read(1024 * 1024).ToString();
	}
	catch (const std::exception& e)
	{
		throw ProxyException(std::string("Motd读取失败: ")+e.what());
	}

}
