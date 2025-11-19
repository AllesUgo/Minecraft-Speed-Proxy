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
#include <regex>
#include "asio/import_asio.h"

/**
 * @brief 代理相关异常类
 * @details 用于在代理操作中抛出异常，包含异常信息
 */
class ProxyException : public std::exception {
private:
	std::string message;
public:
	/**
  * @brief 构造函数
  * @param message 异常信息
  */
	ProxyException(const std::string& message) noexcept;
	/**
  * @brief 获取异常信息
  * @return 异常描述字符串
  */
	const char* what() const noexcept override;
};

/**
 * @brief 用户连接信息结构体
 * @details 包含用户名、UUID、IP、上传字节数、连接时间及客户端/服务端socket
 */
struct User {
	std::string username; ///< 用户名
	std::string uuid; ///< UUID
	std::string ip; ///< IP地址
	std::atomic_uint64_t upload_bytes = 0; ///< 上传字节数
	std::time_t connect_time; ///< 连接时间
	asio::ip::tcp::socket* client, * server; ///< 客户端与服务端socket指针

	/**
  * @brief 构造函数
  * @param client 客户端socket指针
  * @param server 服务端socket指针
  */
	User(asio::ip::tcp::socket* client, asio::ip::tcp::socket* server);
};

/**
 * @brief 用户基本信息结构体
 * @details 用于描述用户的基本信息，不包含连接信息
 */
struct UserInfo {
	std::string username; ///< 用户名
	std::string uuid; ///< UUID
	std::string ip; ///< IP地址
	std::uint64_t upload_bytes; ///< 上传及下载字节数
	std::time_t connect_time; ///< 连接时间
};

/**
 * @brief MOTD（服务器消息）结构体
 * @details 用于管理和生成服务器MOTD信息
 */
struct Motd {
	neb::CJsonObject motd_json; ///< MOTD的JSON对象

	Motd& operator=(const Motd&) = delete;

	/**
  * @brief 设置服务器版本信息
  * @param version_name 版本名称
  * @param protocol 协议号
  */
	void SetVersion(const std::string& version_name, int protocol);

	/**
  * @brief 设置最大玩家数
  * @param n 最大玩家数
  */
	void SetPlayerMaxNumber(int n);

	/**
  * @brief 设置在线玩家数
  * @param n 在线玩家数
  */
	void SetOnlinePlayerNumber(int n);

	/**
  * @brief 设置示例用户列表
  * @param users 用户信息列表
  */
	void SetSampleUsers(std::list<UserInfo> const& users);

	/**
  * @brief 转换为字符串
  * @return MOTD的JSON字符串
  */
	auto ToString() -> std::string;

	/**
  * @brief 从文件加载MOTD
  * @param path 文件路径
  * @return 文件内容字符串
  */
	static auto LoadMotdFromFile(const std::string& path) -> std::string;
};
/**
 * @brief 利用asio库实现的异步高性能代理
 * @details 提供用户连接管理、MOTD设置、用户代理映射等功能
 */
class Proxy
{
public:
	/**
  * @brief 连接控制类
  * @details 用于管理单个连接的状态和信息
  */
	class ConnectionControl
	{
	public:
		/**
   * @brief 构造函数
   * @param connection TCP连接socket
   */
		ConnectionControl(asio::ip::tcp::socket& connection);

		/**
   * @brief 获取用户名
   * @return 用户名字符串
   */
		const std::string& Username(void) const noexcept;

		/**
   * @brief 获取UUID
   * @return UUID字符串
   */
		const std::string& UUID(void) const noexcept;

		/**
   * @brief 获取IP地址
   * @return IP地址字符串
   */
		std::string GetAddress(void) const noexcept;

		/**
   * @brief 获取端口号
   * @return 端口号字符串
   */
		std::string GetPort(void) const noexcept;

		bool isEnableConnect = true; ///< 是否允许连接
		std::string reason; ///< 禁止连接原因

		/**
   * @brief 获取上传字节数F（实际是代理字节数）
   * @return 上传字节数
   */
		std::size_t UploadBytes(void) const noexcept;

		/**
   * @brief 获取连接时间
   * @return 连接时间
   */
		std::time_t ConnectTime(void) const noexcept;

		friend class Proxy;
	private:
		std::string username; ///< 用户名
		std::string uuid; ///< UUID
		asio::ip::tcp::socket& socket; ///< 连接socket
		std::size_t upload_bytes = 0; ///< 上传字节数
		std::time_t connect_time; ///< 连接时间
	};

	/// @brief 连接成功并加入连接池后的回调，抛出异常将导致连接关闭
	RbsLib::Function::Function<void(ConnectionControl&)> on_connected;
	/// @brief 即将断开连接前的回调，连接已不可用，抛出异常无效
	RbsLib::Function::Function<void(ConnectionControl&)> on_disconnect;
	/// @brief 收到登录数据包后的回调，抛出异常将显示在客户端
	RbsLib::Function::Function<void(ConnectionControl&)> on_login;
	/// @brief 用户即将断开连接前的回调，logout先于disconnect，抛出异常导致未定义行为
	RbsLib::Function::Function<void(ConnectionControl&)> on_logout;
	/// @brief 代理开始前的回调
	RbsLib::Function::Function<void(ConnectionControl&)> on_proxy_start;
	/// @brief 代理结束后的回调
	RbsLib::Function::Function<void(ConnectionControl&)> on_proxy_end;
	/// @brief 错误日志输出回调，抛出异常导致未定义行为
	RbsLib::Function::Function<void(ConnectionControl&)> exception_handle;
	/// @brief 日志输出回调，抛出异常导致未定义行为
	RbsLib::Function::Function<void(const char*)> log_output;

	/**
  * @brief 构造函数
  * @param local_address 本地监听地址
  * @param local_port 本地监听端口
  * @param remote_server_addr 远程服务器地址
  * @param remote_server_port 远程服务器端口
  */
	Proxy(const std::string& local_address, std::uint16_t local_port, const std::string& remote_server_addr, std::uint16_t remote_server_port);

	Proxy(const Proxy&) = delete;
	Proxy& operator=(const Proxy&) = delete;

	/**
  * @brief 启动代理服务
  */
	void Start();

	/**
  * @brief 获取代理启动时间
  * @return 启动时间
  */
	std::time_t GetStartTime(void) const noexcept;

	/**
  * @brief 析构函数，将会请求停止代理服务并等待所有连接关闭
  */
	~Proxy() noexcept;

	/**
  * @brief 根据用户名踢出用户
  * @param username 用户名
  */
	void KickByUsername(const std::string& username);

	/**
  * @brief 根据UUID踢出用户
  * @param uuid 用户UUID
  */
	void KickByUUID(const std::string& uuid);

	/**
  * @brief 获取当前在线用户信息列表
  * @return 用户信息列表
  */
	auto GetUsersInfo() -> std::list<UserInfo>;

	/**
  * @brief 异步获取在线用户信息列表（协程调用）
  * @return 用户信息列表
  */
	auto GetUsersInfoAsync() -> asio::awaitable<std::list<UserInfo>>;

	/**
  * @brief 设置MOTD
  * @param motd MOTD的JSON字符串
  */
	void SetMotd(const std::string& motd);

	/**
  * @brief 获取当前MOTD的JSON对象
  * @return MOTD的JSON对象
  */
	neb::CJsonObject GetMotd() const;

	/**
  * @brief 设置最大在线人数
  * @param n 最大人数，-1表示不限制
  */
	void SetMaxPlayer(int n);

	/**
  * @brief 获取最大在线人数
  * @return 最大人数，-1表示不限制
  */
	int GetMaxPlayer(void);

	/**
  * @brief 为指定用户设置代理地址和端口
  * @param username 用户名
  * @param proxy_address 代理地址
  * @param proxy_port 代理端口
  */
	void SetUserProxy(const std::string& username, const std::string& proxy_address, std::uint16_t proxy_port);

	/**
  * @brief 获取所有用户的代理地址和端口映射
  * @return 用户代理映射表
  */
	auto GetUserProxyMap() const -> std::map<std::string, std::pair<std::string, std::uint16_t>>;

	/**
  * @brief 删除指定用户的代理地址和端口映射
  * @param username 用户名
  */
	void DeleteUserProxy(const std::string& username);

	/**
  * @brief 清空所有用户的代理地址和端口映射
  */
	void ClearUserProxy();

	/**
  * @brief 获取默认代理地址和端口
  * @return 地址和端口对
  */
	auto GetDefaultProxy() -> std::pair<std::string, std::uint16_t>;

	/**
  * @brief 测试与远程服务器的ping值
  * @return ping值（ms），-1表示失败
  */
	auto PingTest()const -> std::uint64_t;

	/**
  * @brief 启用域名代理映射
  * @param doman_name 域名模板
  */
	void EnableDomainNameProxy(const std::string& doman_name);


	/**
  * @brief 添加域名代理映射
  * @param doman_pattern 域名匹配得到的字符串
  * @param target_address 目标地址
  * @param target_port 目标端口
  */
	void AddDomainNameProxyMapping(const std::string& domain_pattern,const std::string& target_address,std::uint16_t target_port);


	/**
  * @brief 移除域名代理映射
  * @param domain_pattern 域名匹配得到的字符串
  */
	void RemoveDomainNameProxyMapping(const std::string& domain_pattern);

	/**
  * @brief 禁用域名代理映射
  */
	void DisableDomainNameProxy();




protected:
	asio::awaitable<void> AcceptLoop(asio::ip::tcp::acceptor& acceptor);
	asio::awaitable<void> HandleConnection(asio::ip::tcp::socket socket);
	asio::awaitable<void> ForwardData(asio::ip::tcp::socket& client_socket, asio::ip::tcp::socket& server_socket, User& user_control) noexcept;

	std::string local_address; ///< 本地监听地址
	std::string remote_server_addr; ///< 远程服务器地址
	std::uint16_t local_port; ///< 本地监听端口
	std::uint16_t remote_server_port; ///< 远程服务器端口
	std::atomic_uint32_t max_player = -1; ///< 最大在线人数
	std::map<std::string, std::shared_ptr<User>> users; ///< 在线用户映射
	asio::io_context io_context; ///< IO上下文
	asio::strand<asio::io_context::executor_type> strand; ///< strand对象
	std::vector<std::thread> io_threads; ///< IO线程池
	std::unique_ptr<asio::ip::tcp::acceptor> acceptor; ///< 连接接收器
	Motd motd; ///< MOTD对象
	std::map<std::string, std::pair<std::string, std::uint16_t>> user_proxy_map; ///< 用户代理映射
	std::optional<std::pair<std::regex /*域名匹配模板*/, std::map<std::string, std::pair<std::string,std::uint16_t>>>/*映射表*/> dn_proxy_map; ///< 域名代理映射
	std::list<asio::ip::tcp::socket*> connections; ///< 活动连接列表
	std::time_t start_time = 0; ///< 代理启动时间
};

#endif // !PROXY_H
