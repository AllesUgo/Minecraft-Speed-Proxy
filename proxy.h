#pragma once
#ifndef PROXY_H
#define PROXY_H
#include "json/CJsonObject.h"
#include "rbslib/Function.h"
#include "rbslib/Network.h"
#include "rbslib/TaskPool.h"
#include <atomic>
#include <exception>
#include <list>
#include <map>
#include <memory>
#include <shared_mutex>
#include "asio/import_asio.h"

class ProxyException : public std::exception {
private: 
	std::string message;
	public:
		ProxyException(const std::string& message) noexcept;
		const char* what() const noexcept override;
};
struct User {
	std::string username;
	std::string uuid;
	std::string ip;
	std::atomic_uint64_t upload_bytes = 0;
	std::time_t connect_time;
	asio::ip::tcp::socket* client, *server;
	User(asio::ip::tcp::socket* client, asio::ip::tcp::socket* server);
};

/*描述用户信息，不包括连接信息等，用于返回*/
struct UserInfo {
    /**
     * @brief 用户信息结构体
     * 
     * 用于描述用户的基本信息，包括用户名、UUID、IP地址、上传字节数和连接时间。
     */
    std::string username; /**< 用户名 */
    std::string uuid; /**< UUID */
    std::string ip; /**< IP地址 */
    std::uint64_t upload_bytes; /**< 上传及下载字节数 */
    std::time_t connect_time; /**< 连接时间 */
};

struct Motd {
	neb::CJsonObject motd_json;
	Motd& operator=(const Motd&) = delete;
	void SetVersion(const std::string& version_name, int protocol);
	void SetPlayerMaxNumber(int n);
	void SetOnlinePlayerNumber(int n);
	void SetSampleUsers(std::list<UserInfo> const& users);
	auto ToString() -> std::string;
	static auto LoadMotdFromFile(const std::string& path) -> std::string;
};

/*利用asio库实现的异步高性能代理*/
class Proxy 
{
public:
	class ConnectionControl
	{
	public:
		ConnectionControl(asio::ip::tcp::socket& connection);
		const std::string& Username(void) const noexcept;
		const std::string& UUID(void) const noexcept;
		std::string GetAddress(void) const noexcept;
		std::string GetPort(void) const noexcept;
		bool isEnableConnect = true;
		std::string reason;
		std::size_t UploadBytes(void) const noexcept;
		std::time_t ConnectTime(void) const noexcept;
		friend class Proxy;
	private:
		std::string username;
		std::string uuid;
		asio::ip::tcp::socket& socket;
		std::size_t upload_bytes = 0;
		std::time_t connect_time;
	};


	//以下为回调函数，均不保证线程安全，需要注意线程安全问题
	RbsLib::Function::Function<void(ConnectionControl&)> on_connected;//发生在连接成功并将连接加入连接池后,抛出任何异常将导致连接关闭
	RbsLib::Function::Function<void(ConnectionControl&)> on_disconnect;//发生在即将与客户端断开连接前，此时连接已不可用，不允许在连接上收发数据，仅用于标识连接。在连接上收发将导致异常。抛出异常无效
	RbsLib::Function::Function<void(ConnectionControl&)> on_login;//发生在收到客户端的登录数据包后，用户未加入在线用户列表，login抛出的异常将会显示在客户端
	RbsLib::Function::Function<void(ConnectionControl&)> on_logout;//发生在用户即将断开连接之前，用户已从在线用户列表中移除并且连接未从连接池断开，logout要求与disconnect要求一致，且保证logout先于disconnect。logout抛出异常将导致未定义行为
	RbsLib::Function::Function<void(ConnectionControl&)> exception_handle;//用于输出错误日志，error_message_callback抛出的异常将导致未定义行为
	RbsLib::Function::Function<void(const char*)> log_output;//用于输出日志，log_output抛出的异常将导致未定义行为

	Proxy(const std::string& local_address, std::uint16_t local_port, const std::string& remote_server_addr, std::uint16_t);
	Proxy(const Proxy&) = delete;
	Proxy& operator=(const Proxy&) = delete;
	void Start();
	//以下函数禁止在回调函数中调用
	void KickByUsername(const std::string& username);
	void KickByUUID(const std::string& uuid);
	auto GetUsersInfo() -> std::list<UserInfo>;
	auto GetUsersInfoAsync() -> asio::awaitable<std::list<UserInfo>>;
	void SetMotd(const std::string& motd);
	void SetMaxPlayer(int n);
	int GetMaxPlayer(void);
	void SetUserProxy(const std::string& username, const std::string& proxy_address, std::uint16_t proxy_port);
	auto GetUserProxyMap() const -> std::map<std::string, std::pair<std::string, std::uint16_t>>;
	void DeleteUserProxy(const std::string& username);
	void ClearUserProxy();
	auto GetDefaultProxy() -> std::pair<std::string, std::uint16_t>;//获取默认代理地址,first:address,second:port
	auto PingTest()const -> std::uint64_t;
	~Proxy() noexcept;
protected:
	asio::awaitable<void> AcceptLoop(asio::ip::tcp::acceptor& acceptor);
	asio::awaitable<void> HandleConnection(asio::ip::tcp::socket socket);
	asio::awaitable<void> ForwardData(asio::ip::tcp::socket& client_socket, asio::ip::tcp::socket& server_socket, User& user_control) noexcept;
	std::string local_address;
	std::string remote_server_addr;
	std::uint16_t local_port;
	std::uint16_t remote_server_port;
	std::atomic_uint32_t max_player = -1;
	std::map<std::string, std::shared_ptr<User>> users;
	asio::io_context io_context;
	asio::strand<asio::io_context::executor_type> strand;
	std::vector<std::thread> io_threads;
	std::unique_ptr<asio::ip::tcp::acceptor> acceptor;
	Motd motd;
	std::map<std::string, std::pair<std::string, std::uint16_t>> user_proxy_map;
	std::list<asio::ip::tcp::socket*> connections;
};


#endif // !PROXY_H
