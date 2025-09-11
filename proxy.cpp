#include "datapackage.h"
#include "proxy.h"
#include <chrono>
#include <iostream>
#include <mutex>
#include "rbslib/Streams.h"
#include <time.h>

class AsyncInputSocketStream : public RbsLib::Streams::IAsyncInputStream
{
private:
	asio::ip::tcp::socket& socket;
public:
	AsyncInputSocketStream(asio::ip::tcp::socket& socket) : socket(socket) {}
	~AsyncInputSocketStream() noexcept {}
	asio::awaitable<std::int64_t> ReadAsync(void* buffer, std::int64_t size) override
	{
		std::size_t n = co_await socket.async_read_some(asio::buffer(buffer, size), asio::use_awaitable);
		co_return n;
	}
	asio::awaitable<const RbsLib::Buffer*> ReadAsync(RbsLib::Buffer& buffer, std::int64_t size = 0) override
	{
		buffer.Resize(size);
		int64_t bytesRead = co_await socket.async_read_some(asio::buffer((void*)buffer.Data(), size), asio::use_awaitable);
		buffer.SetLength(bytesRead);
		buffer.Resize(bytesRead);
		co_return &buffer;
	}
};

std::size_t Proxy::ConnectionControl::UploadBytes(void) const noexcept
{
	return this->upload_bytes;
}

std::time_t Proxy::ConnectionControl::ConnectTime(void) const noexcept
{
	return this->connect_time;
}

auto Proxy::PingTest() const -> std::uint64_t
{
	return asio::co_spawn(this->strand, [this]() -> asio::awaitable<std::uint64_t> {
		try
		{
			// 解析主机名和端口
			asio::ip::tcp::resolver resolver(co_await asio::this_coro::executor);
			auto endpoints = co_await resolver.async_resolve(this->remote_server_addr, std::to_string(this->remote_server_port), asio::use_awaitable);

			// 创建socket并连接
			asio::ip::tcp::socket remote_server(co_await asio::this_coro::executor);
			co_await asio::async_connect(remote_server, endpoints, asio::use_awaitable);
			remote_server.set_option(asio::ip::tcp::no_delay(true));

			// 创建握手数据包
			HandshakeDataPack handshake_data_pack;
			handshake_data_pack.id = 0;
			handshake_data_pack.server_address = RbsLib::DataType::String(this->remote_server_addr);
			handshake_data_pack.server_port = this->remote_server_port;
			handshake_data_pack.next_state = 1;
			handshake_data_pack.protocol_version = 754;
			auto handshake_buffer = handshake_data_pack.ToBuffer();
			co_await remote_server.async_send(asio::buffer(handshake_buffer.Data(), handshake_buffer.GetLength()), asio::use_awaitable);

			AsyncInputSocketStream stream(remote_server);
			
			// 发送状态请求
			StatusRequestDataPack status_request_data_pack;
			auto status_buffer = status_request_data_pack.ToBuffer();
			co_await remote_server.async_send(asio::buffer(status_buffer.Data(), status_buffer.GetLength()), asio::use_awaitable);
			
			// 接收状态并丢弃
			co_await DataPack::ReadFullData(stream);
			
			// ping
			PingDataPack ping_data_pack, pong_data_pack;
			ping_data_pack.payload = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			auto start = std::chrono::system_clock::now();
			auto ping_buffer = ping_data_pack.ToBuffer();
			co_await remote_server.async_send(asio::buffer(ping_buffer.Data(), ping_buffer.GetLength()), asio::use_awaitable);
			co_await pong_data_pack.ParseFromInputStream(stream);
			auto end = std::chrono::system_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
			
			if (pong_data_pack.payload != ping_data_pack.payload) 
				throw ProxyException("Ping test failed.");
			
			co_return duration.count();
		}
		catch (const std::exception& e)
		{
			throw ProxyException(std::string("Ping test failed: ") + e.what());
		}
	}, asio::use_future).get();
}

std::time_t Proxy::GetStartTime(void) const noexcept
{
	return this->start_time;
}

Proxy::~Proxy() noexcept
{
	//正常退出，关闭acceptor和所有连接，等待所有协程结束后其所属线程退出，然后关闭io_context
	try
	{
		this->acceptor->close();
		asio::co_spawn(this->strand,
			[this]() -> asio::awaitable<void> {
				for (auto& conn : this->connections)
				{
					try
					{
						conn->shutdown(asio::ip::tcp::socket::shutdown_both);
					}
					catch (...) {}
				}
				co_return;
			}(),
			asio::use_future).get();
	}
	catch (const std::exception* ex) 
	{
		this->log_output((std::string("关闭监听器或连接时发生异常: ") + ex->what()).c_str());
	}
	for (auto& thread : this->io_threads)
	{
		if (thread.joinable())
			thread.join();
	}
	try
	{
		this->io_context.stop();
	}
	catch (const std::exception* ex)
	{
		this->log_output((std::string("停止IO上下文时发生异常: ") + ex->what()).c_str());
	}
}

asio::awaitable<void> Proxy::AcceptLoop(asio::ip::tcp::acceptor& acceptor)
{
	try
	{
		while (true)
		{
			auto socket = co_await acceptor.async_accept(asio::use_awaitable);
			asio::co_spawn(socket.get_executor(),
				this->HandleConnection(std::move(socket)),
				asio::detached);
		}
	}
	catch (std::system_error const& ex)
	{
		if (ex.code() != asio::error::operation_aborted) {
			throw;
		}
	}
}

asio::awaitable<void> Proxy::HandleConnection(asio::ip::tcp::socket socket)
{
	//std::string remote_endpoint_addr = socket.remote_endpoint().address().to_string();
	std::time_t connect_time = std::time(nullptr);
	//立即将连接加入连接池
	co_await asio::dispatch(strand, asio::use_awaitable);
	this->connections.push_back(&socket);
	try
	{
		this->log_output(("New connection from " + socket.remote_endpoint().address().to_string() + ":" + std::to_string(socket.remote_endpoint().port())).c_str());
		ConnectionControl user_control(socket);
		user_control.connect_time = connect_time;
		int connection_status = 0;//未握手
		//必须先握手
		std::string remote_server_suffix;
		HandshakeDataPack handshake_data_pack;
		AsyncInputSocketStream stream(socket);
		co_await handshake_data_pack.ParseFromInputStream(stream);
		connection_status = handshake_data_pack.next_state.Value();
		bool is_continue_loop = true;
		while (is_continue_loop)
		{
			switch (connection_status)
			{
			case 1: {
				RbsLib::Buffer buffer = co_await DataPack::ReadFullData(stream);
				switch (NoCompressionDataPack::GetID(buffer))
				{
				case 0: {
					//状态请求，直接响应
					this->log_output(("Status request from " + socket.remote_endpoint().address().to_string() + ":" + std::to_string(socket.remote_endpoint().port())).c_str());
					StatusResponseDataPack status_response_data_pack;
					co_await asio::dispatch(strand, asio::use_awaitable); //确保在线程安全的环境中访问用户数据
					auto motd = this->motd;
					motd.SetOnlinePlayerNumber(this->users.size());
					motd.SetSampleUsers(co_await this->GetUsersInfoAsync());
					motd.SetVersion("", handshake_data_pack.protocol_version.Value());
					this->max_player != -1 ? motd.SetPlayerMaxNumber(this->max_player) : motd.SetPlayerMaxNumber(9999999);
					status_response_data_pack.json_response = RbsLib::DataType::String(motd.ToString());
					co_await socket.async_send(asio::buffer(status_response_data_pack.ToBuffer().Data(), status_response_data_pack.ToBuffer().GetLength()), asio::use_awaitable);
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
					co_await socket.async_send(asio::buffer(pong_data_pack.ToBuffer().Data(), pong_data_pack.ToBuffer().GetLength()), asio::use_awaitable);
					this->log_output(("Ping from " + socket.remote_endpoint().address().to_string() + ":" + std::to_string(socket.remote_endpoint().port()) + ", payload: " + std::to_string(ping_data_pack.payload)).c_str());
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
				this->log_output(("Login request from " + socket.remote_endpoint().address().to_string() + ":" + std::to_string(socket.remote_endpoint().port())).c_str());
				co_await asio::dispatch(strand, asio::use_awaitable); //确保在线程安全的环境中访问用户数据
				if (this->max_player != -1 && this->users.size() >= this->max_player) {
					LoginFailureDataPack login_failed_data_pack("Server is full.");
					co_await socket.async_send(asio::buffer(login_failed_data_pack.ToBuffer().Data(), login_failed_data_pack.ToBuffer().GetLength()), asio::use_awaitable);
					this->log_output(("Refused connection " + socket.remote_endpoint().address().to_string() + ":" + std::to_string(socket.remote_endpoint().port()) + ", reason: Server is full.").c_str());
					throw ProxyException("Server is full.");
				}
				StartLoginDataPack start_login_data_pack;
				auto login_start_row_packet = co_await DataPack::ReadFullData(stream);
				RbsLib::Streams::BufferInputStream bis(login_start_row_packet);
				start_login_data_pack.ParseFromInputStream(bis);
				//检查是否是FML登录
				if (handshake_data_pack.server_address.size() != std::strlen(handshake_data_pack.server_address.c_str()))
				{
					remote_server_suffix = handshake_data_pack.server_address.substr(std::strlen(handshake_data_pack.server_address.c_str()));
					this->log_output(("FML login detected on " + socket.remote_endpoint().address().to_string() + ":" + std::to_string(socket.remote_endpoint().port()) + ", suffix: " + remote_server_suffix).c_str());
				}
				//调用登录回调
				user_control.username = start_login_data_pack.user_name;
				if (start_login_data_pack.have_uuid)
				{
					user_control.uuid = start_login_data_pack.GetUUID();
				}
				co_await asio::dispatch(strand, asio::use_awaitable);//串行化
				this->on_login(user_control);
				if (user_control.isEnableConnect == false)
				{
					LoginFailureDataPack login_failed_data_pack(user_control.reason);
					auto buffer = login_failed_data_pack.ToBuffer();
					co_await socket.async_send(asio::buffer(buffer.Data(), buffer.GetLength()), asio::use_awaitable);
					throw ProxyException(std::string("Callback disable user login: ") + user_control.reason);
				}

				//连接远程服务器
				std::string remote_server_addr_real;
				std::uint32_t remote_server_port_real;
				co_await asio::dispatch(strand, asio::use_awaitable); //确保在线程安全的环境中访问用户数据
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
				asio::ip::tcp::socket remote_server(socket.get_executor());
				try
				{
					// 解析主机名和端口
					asio::ip::tcp::resolver resolver(socket.get_executor());
					auto endpoints = co_await resolver.async_resolve(remote_server_addr_real, std::to_string(remote_server_port_real), asio::use_awaitable);

					// 尝试连接每个解析到的端点
					co_await asio::async_connect(remote_server, endpoints, asio::use_awaitable);
					this->log_output(("Connected to remote server " + remote_server_addr_real + ":" + std::to_string(remote_server_port_real)).c_str());
				}
				catch (const std::exception& e)
				{
					throw; // 重新抛出异常
				}
				remote_server.set_option(asio::ip::tcp::no_delay(true));
				socket.set_option(asio::ip::tcp::no_delay(true));
				auto user_ptr = std::make_shared<User>(&socket, &remote_server);

				user_ptr->username = start_login_data_pack.user_name;
				user_ptr->uuid = start_login_data_pack.GetUUID();
				user_ptr->ip = socket.remote_endpoint().address().to_string();
				user_ptr->connect_time = std::time(nullptr);
				//加入用户池
				co_await asio::dispatch(strand, asio::use_awaitable); //串行化
				if (this->users.find(start_login_data_pack.user_name) != this->users.end())
				{
					LoginFailureDataPack login_failed_data_pack(start_login_data_pack.user_name + " already online");
					RbsLib::Buffer buffer = login_failed_data_pack.ToBuffer();
					co_await socket.async_send(asio::buffer(buffer.Data(), buffer.GetLength()), asio::use_awaitable);
					throw ProxyException("User already online: " + start_login_data_pack.user_name);
				}
				this->users[user_ptr->username] = user_ptr;
				bool error_occurred = false;
				//将远程服务器也加入连接池
				co_await asio::dispatch(strand, asio::use_awaitable); //串行化
				this->connections.push_back(&remote_server);
				try
				{
					//此阶段若要断开连接需要从用户表中删除用户
					//发送握手申请
					handshake_data_pack.server_address = RbsLib::DataType::String(remote_server_addr_real + remote_server_suffix);
					auto buffer = handshake_data_pack.ToBuffer();
					co_await remote_server.async_send(asio::buffer(buffer.Data(), buffer.GetLength()), asio::use_awaitable);
					//发送登录请求
					co_await remote_server.async_send(asio::buffer(login_start_row_packet.Data(), login_start_row_packet.GetLength()), asio::use_awaitable);
				}
				catch (...)
				{
					error_occurred = true;
				}
				if (!error_occurred)
				{
					//无错误进入转发，期间一般不会抛出异常，出现异常一般都是致命错误
					//将连接加入连接池，用于退出服务器时关闭
					co_await asio::dispatch(strand, asio::use_awaitable); //串行化
					this->connections.push_back(&remote_server);
					this->connections.push_back(&socket);
					//开始转发数据
					{
						try
						{
							this->on_proxy_start(user_control); //调用代理开始回调
							this->log_output(("Forwarding data between " + socket.remote_endpoint().address().to_string() + ":" + std::to_string(socket.remote_endpoint().port()) + " and " + remote_server.remote_endpoint().address().to_string() + ":" + std::to_string(remote_server.remote_endpoint().port())).c_str());
							using namespace asio::experimental::awaitable_operators;
							co_await(ForwardData(socket, remote_server, *user_ptr) && ForwardData(remote_server, socket, *user_ptr));
							this->log_output(("Forwarding data between " + socket.remote_endpoint().address().to_string() + ":" + std::to_string(socket.remote_endpoint().port()) + " and " + remote_server.remote_endpoint().address().to_string() + ":" + std::to_string(remote_server.remote_endpoint().port()) + " ended.").c_str());
							this->on_proxy_end(user_control); //调用代理结束回调
						}
						catch (const std::exception& ex)
						{
							//std::cout <<ex.what() << std::endl;
						}
					}
					//转发结束，关闭连接
					co_await asio::dispatch(strand, asio::use_awaitable); //串行化
					this->connections.remove(&remote_server);
					this->connections.remove(&socket);
				}
				//清理资源
				co_await asio::dispatch(strand, asio::use_awaitable); //串行化
				this->connections.remove(&remote_server);//将远程服务器连接从连接池删除
				this->users.erase(start_login_data_pack.user_name);//从用户池中删除用户
				user_control.username = user_ptr->username;
				user_control.uuid = user_ptr->uuid;
				user_control.upload_bytes = user_ptr->upload_bytes;
				this->on_logout(user_control); //调用登出回调
				is_continue_loop = false; //退出循环
			}
				  break;
			default:
				this->log_output(("Invalid connection status: " + std::to_string(connection_status) + " from "+ socket.remote_endpoint().address().to_string()+":"+std::to_string(socket.remote_endpoint().port())).c_str());
				throw ProxyException("Invalid connection status.");
			}
		}
	}
	catch (...) {}
	//删除连接池中的连接
	co_await asio::dispatch(strand, asio::use_awaitable); //串行化
	this->connections.remove(&socket);
}


asio::awaitable<void> Proxy::ForwardData(
	asio::ip::tcp::socket& from_socket,
	asio::ip::tcp::socket& to_socket,
	User& user_control) noexcept
{
	std::unique_ptr<std::uint8_t[]> buffer = std::make_unique<std::uint8_t[]>(10240);
	try
	{
		while (true)
		{
			std::size_t n = co_await from_socket.async_receive(asio::buffer(buffer.get(), 10240), asio::use_awaitable);
			if (n == 0)
				break;
			co_await to_socket.async_send(asio::buffer(buffer.get(), n), asio::use_awaitable);
			user_control.upload_bytes += n;
		}
	}
	catch (...)
	{
		//忽略转发时的异常
	}
	try
	{
		//禁用两个方向的连接
		user_control.client->shutdown(asio::ip::tcp::socket::shutdown_both);
	}
	catch (const std::exception& e)
	{
		//忽略禁用连接时的异常
	}
	try
	{
		user_control.server->shutdown(asio::ip::tcp::socket::shutdown_both);
	}
	catch (const std::exception& e)
	{
		//忽略禁用连接时的异常
	}
}


ProxyException::ProxyException(const std::string& message) noexcept
	: message(message)
{
}

const char* ProxyException::what() const noexcept
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
	if (this->motd_json["players"]["sample"].GetArraySize() == 0)
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
		throw ProxyException(std::string("Motd读取失败: ") + e.what());
	}

}


Proxy::Proxy(const std::string& local_address, std::uint16_t local_port, const std::string& remote_server_addr, std::uint16_t remote_server_port)
	:local_address(local_address), local_port(local_port), remote_server_addr(remote_server_addr), remote_server_port(remote_server_port), strand(asio::make_strand(io_context.get_executor()))
{
	this->io_threads.resize(std::thread::hardware_concurrency());
	
}

void Proxy::Start() {
	auto addr = asio::ip::make_address(this->local_address);
	asio::ip::tcp::endpoint endpoint(addr, this->local_port);

	// 1. 先创建 acceptor（未绑定）
	this->acceptor = std::make_unique<asio::ip::tcp::acceptor>(this->io_context);

	// 2. 设置选项（必须在 bind 前）
	this->acceptor->open(endpoint.protocol()); // 显式打开协议
	if (endpoint.protocol() == asio::ip::tcp::v6()) {
		this->acceptor->set_option(asio::ip::v6_only(false)); // 关键：在 bind 前设置
	}
	this->acceptor->set_option(asio::socket_base::reuse_address(true));

	// 3. 绑定到端点
	this->acceptor->bind(endpoint);

	// 4. 开始监听
	this->acceptor->listen();

	// 发布任务
	asio::co_spawn(this->io_context, this->AcceptLoop(*this->acceptor), asio::detached);
	// 启动线程池
	for (auto& thread : this->io_threads) {
		thread = std::thread([this]() {
			this->io_context.run();
			});
	}
	
	this->start_time = std::time(nullptr);
}

void Proxy::KickByUsername(const std::string& username)
{

	asio::co_spawn(this->strand, [this, username]() -> asio::awaitable<void> {

		co_await asio::dispatch(this->strand, asio::use_awaitable);
		auto it = this->users.find(username);
		if (it != this->users.end())
		{
			it->second->client->shutdown(asio::ip::tcp::socket::shutdown_both);
		}
		else
		{
			throw std::runtime_error(username + " not online");
		}
		}, asio::use_future).get();
}

void Proxy::KickByUUID(const std::string& uuid)
{
	asio::co_spawn(this->strand, [this, &uuid]() -> asio::awaitable<void> {
		co_await asio::dispatch(this->strand, asio::use_awaitable);
		for (auto it = this->users.begin(); it != this->users.end(); ++it)
		{
			if (it->second->uuid == uuid)
			{
				it->second->client->shutdown(asio::ip::tcp::socket::shutdown_both);
				break;
			}
		}
		}, asio::use_future).get();
}

auto Proxy::GetUsersInfo() -> std::list<UserInfo>
{
	return asio::co_spawn(this->strand, [this]() -> asio::awaitable<std::list<UserInfo>> {
		std::list<UserInfo> users_info;
		co_await asio::dispatch(this->strand, asio::use_awaitable);
		for (auto& user : this->users)
		{
			UserInfo user_info;
			user_info.username = user.second->username;
			user_info.uuid = user.second->uuid;
			user_info.ip = user.second->ip;
			user_info.connect_time = user.second->connect_time;
			user_info.upload_bytes = user.second->upload_bytes;
			users_info.push_back(user_info);
		}
		co_return users_info;
		}, asio::use_future).get();
}

auto Proxy::GetUsersInfoAsync() -> asio::awaitable<std::list<UserInfo>>
{
	std::list<UserInfo> users_info;
	co_await asio::dispatch(this->strand, asio::use_awaitable);
	for (auto& user : this->users)
	{
		UserInfo user_info;
		user_info.username = user.second->username;
		user_info.uuid = user.second->uuid;
		user_info.ip = user.second->ip;
		user_info.connect_time = user.second->connect_time;
		user_info.upload_bytes = user.second->upload_bytes;
		users_info.push_back(user_info);
	}
	co_return users_info;
}

void Proxy::SetMotd(const std::string& motd)
{
	asio::co_spawn(this->strand, [this, motd]() -> asio::awaitable<void> {
		co_await asio::dispatch(this->strand, asio::use_awaitable);
		this->motd.motd_json = neb::CJsonObject(motd);
		}, asio::use_future).get();
}

neb::CJsonObject Proxy::GetMotd() const
{
	return asio::co_spawn(this->strand, [this]() -> asio::awaitable<neb::CJsonObject> {
		co_await asio::dispatch(this->strand, asio::use_awaitable);
		co_return this->motd.motd_json;
		}, asio::use_future).get();
}

void Proxy::SetMaxPlayer(int n)
{
	this->max_player.store(n);//原子变量不需要发送到协程
}

int Proxy::GetMaxPlayer(void)
{
	return this->max_player.load();//原子变量不需要发送到协程
}

void Proxy::SetUserProxy(const std::string& username, const std::string& proxy_address, std::uint16_t proxy_port)
{
	asio::co_spawn(this->strand, [this, username, proxy_address, proxy_port]() -> asio::awaitable<void> {
		co_await asio::dispatch(this->strand, asio::use_awaitable);
		this->user_proxy_map[username] = std::make_pair(proxy_address, proxy_port);
		}, asio::use_future).get();
}

auto Proxy::GetUserProxyMap() const -> std::map<std::string, std::pair<std::string, std::uint16_t>>
{
	return asio::co_spawn(this->strand, [this]() -> asio::awaitable<std::map<std::string, std::pair<std::string, std::uint16_t>>> {
		co_await asio::dispatch(this->strand, asio::use_awaitable);
		co_return this->user_proxy_map;
		}, asio::use_future).get();
}

void Proxy::DeleteUserProxy(const std::string& username)
{
	asio::co_spawn(this->strand, [this, username]() -> asio::awaitable<void> {
		co_await asio::dispatch(this->strand, asio::use_awaitable);
		this->user_proxy_map.erase(username);
		}, asio::use_future).get();
}

void Proxy::ClearUserProxy()
{
	asio::co_spawn(this->strand, [this]() -> asio::awaitable<void> {
		co_await asio::dispatch(this->strand, asio::use_awaitable);
		this->user_proxy_map.clear();
		}, asio::use_future).get();
}

auto Proxy::GetDefaultProxy() -> std::pair<std::string, std::uint16_t>
{
	return std::make_pair(this->remote_server_addr, this->remote_server_port);
}

Proxy::ConnectionControl::ConnectionControl(asio::ip::tcp::socket& connection)
	: socket(connection)
{
}

const std::string& Proxy::ConnectionControl::Username(void) const noexcept
{
	return this->username;
}

const std::string& Proxy::ConnectionControl::UUID(void) const noexcept
{
	return this->uuid;
}

User::User(asio::ip::tcp::socket* client, asio::ip::tcp::socket* server)
	:client(client), server(server)
{
}

std::string Proxy::ConnectionControl::GetAddress(void) const noexcept
{
	try
	{
		return this->socket.remote_endpoint().address().to_string();
	}
	catch (...)
	{
		return "";
	}
}

std::string Proxy::ConnectionControl::GetPort(void) const noexcept
{
	try
	{
		return this->socket.remote_endpoint().port() == 0 ? "0" : std::to_string(this->socket.remote_endpoint().port());
	}
	catch (...)
	{
		return 0;
	}
}
