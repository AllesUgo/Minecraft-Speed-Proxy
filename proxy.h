#pragma once
#ifndef PROXY_H
#define PROXY_H
#include "rbslib/Network.h"
#include "rbslib/TaskPool.h"
#include "rbslib/Function.h"
#include <map>
#include <list>
#include <memory>
#include <shared_mutex>
#include <atomic>
#include "json/CJsonObject.h"

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
	RbsLib::Network::TCP::TCPConnection client, server;
	User(const RbsLib::Network::TCP::TCPConnection& client, const RbsLib::Network::TCP::TCPConnection& server);
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

class Proxy {

public:

	class CallbackException : public std::exception {
		//定义用于在回调中抛出的异常，该异常用于指示回调函数返回状态
	protected:
		std::string message;
	public:
		CallbackException(const std::string& message) noexcept;
		const char* what() const noexcept override;
	};

	//以下为回调函数，均不保证线程安全，需要注意线程安全问题
	RbsLib::Function::Function<void(const RbsLib::Network::TCP::TCPConnection& client)> on_connected;//发生在连接成功并将连接加入连接池后,抛出任何异常将导致连接关闭
	RbsLib::Function::Function<void(const RbsLib::Network::TCP::TCPConnection& client)> on_disconnect;//发生在即将与客户端断开连接前，此时连接已不可用，不允许在连接上收发数据，仅用于标识连接。在连接上收发将导致异常。抛出异常无效
	RbsLib::Function::Function<void(const RbsLib::Network::TCP::TCPConnection& client, const std::string& username, const std::string& uuid)> on_login;//发生在收到客户端的登录数据包后，用户未加入在线用户列表，login抛出的异常将会显示在客户端
	RbsLib::Function::Function<void(const RbsLib::Network::TCP::TCPConnection& client, const UserInfo& userinfo)> on_logout;//发生在用户即将断开连接之前，用户已从在线用户列表中移除并且连接未从连接池断开，logout要求与disconnect要求一致，且保证logout先于disconnect。logout抛出异常将导致未定义行为

	Proxy(bool is_ipv6_local, const std::string& local_address, std::uint16_t local_port, bool is_ipv6_remote, const std::string& remote_server_addr, std::uint16_t);
	Proxy(const Proxy&) = delete;
	Proxy& operator=(const Proxy&) = delete;
	void Start();
	void KickByUsername(const std::string& username);
	void KickByUUID(const std::string& uuid);
	auto GetUsersInfo() -> std::list<UserInfo>;
	void SetMotd(const std::string& motd);
	void SetMaxPlayer(int n);
	~Proxy() noexcept;
protected:
	int max_player = -1;
	std::shared_mutex motd_mutex;
	Motd motd;
	std::list<RbsLib::Network::TCP::TCPConnection> connections;
	std::map<std::string, std::shared_ptr<User>> users;
	RbsLib::Network::TCP::TCPServer local_server;
	std::string remote_server_addr;
	std::uint16_t remote_server_port;
	bool is_ipv6_remote;
	std::shared_mutex global_mutex;
	RbsLib::Thread::TaskPool thread_pool = 11;
};
#endif // !PROXY_H
