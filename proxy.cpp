#include "proxy.h"
#include "datapackage.h"
#include <iostream>

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
				this->task_pool.Run([this, connection]() {
					try {
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
								std::cout << start_login_data_pack.user_name.c_str();
								if (start_login_data_pack.have_uuid)
									std::cout <<":" << start_login_data_pack.GetUUID();
								std::cout<<std::endl;
								//连接远程服务器
								auto remote_server = is_ipv6_remote ? RbsLib::Network::TCP::TCPClient::Connect6(remote_server_addr, remote_server_port) : RbsLib::Network::TCP::TCPClient::Connect(remote_server_addr, remote_server_port);
								int flag = 1;
								remote_server.SetSocketOption(IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
								flag = 1;
								connection.SetSocketOption(IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
								try {
									//发送握手申请
									remote_server.Send(handshake_data_pack.ToBuffer());
									//发送登录请求
									remote_server.Send(start_login_data_pack.ToBuffer());
									//进入代理
									RbsLib::Buffer buffer(1024);
									this->thread_pool.Run([this, connection, remote_server]() {
										try
										{
											RbsLib::Buffer buffer(1024);
											while (true) {
												remote_server.Recv(buffer);
												connection.Send(buffer);
											}
										}
										catch (const std::exception& e)
										{
											std::cerr << e.what() << std::endl;
											connection.DisableSocket();
											remote_server.DisableSocket();
										}
										});
									RbsLib::Buffer b(1024);
									while (true) {
										connection.Recv(b);
										remote_server.Send(b);
									}
								}
								catch (...) {
									remote_server.DisableSocket();
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
						std::cerr << e.what() << std::endl;
						connection.DisableSocket();
					}
					});
			}
		}
		catch (const std::exception& ex){
			std::cerr << ex.what() << std::endl;
		}
		});
		
}

ProxyException::ProxyException(const std::string& message) noexcept
	: message(message)
{
}

const char* ProxyException::what() const noexcept
{
	return this->message.c_str();
}
