#pragma once
#include <memory>
#include <chrono>
#include "rbslib/Network.h"
#include "proxy.h"
#include "json/CJsonObject.h"
#include <list>
#include <shared_mutex>
#include "asio/import_asio.h"


/*
* ��ҳ���Ʒ����࣬�ṩ����WebAPI���Ʒ������Ĺ���
*/
class WebControlServer
{
protected:
	RbsLib::Network::HTTP::HTTPServer server;
	std::string user_token;
	//token����ʱ��
	std::chrono::system_clock::time_point token_expiry_time;
	std::shared_ptr<std::string> user_password;
	bool is_request_stop = false;
	std::list<std::pair<std::time_t, std::string>> logs;
	int max_log_size = 100; //�����־����
	std::shared_mutex log_mutex; //��־������
	asio::io_context io_context;
	std::map<std::time_t, uint32_t> time_online_users;
	std::shared_mutex time_online_users_mutex;

	asio::awaitable<void> TimeTaskUsers(std::shared_ptr<Proxy>& proxy_client);

	static void SendErrorResponse(const RbsLib::Network::TCP::TCPConnection& connection, int status_code, const std::string& message = "");
	static void SendErrorResponse(const RbsLib::Network::TCP::TCPConnection& connection, const neb::CJsonObject& json, int http_status_code = 200);
	static void SendSuccessResponse(const RbsLib::Network::TCP::TCPConnection& connection, const neb::CJsonObject& json);
	static bool CheckToken(const std::string& cookie,const std::string& token,const std::chrono::system_clock::time_point& token_expiry_time);
	static void GetOnlineUsers(neb::CJsonObject& response, const std::shared_ptr<Proxy>& proxy_client);
	static void GetWhiteList(neb::CJsonObject& response, const std::shared_ptr<Proxy>& proxy_client);
	static void GetBlackList(neb::CJsonObject& response, const std::shared_ptr<Proxy>& proxy_client);
	static bool AddBlacklistUser(neb::CJsonObject& response,const neb::CJsonObject& request, const std::shared_ptr<Proxy>& proxy_client);
	static bool RemoveBlacklistUser(neb::CJsonObject& response, const neb::CJsonObject& request, const std::shared_ptr<Proxy>& proxy_client);
	static bool AddWhitelistUser(neb::CJsonObject& response, const neb::CJsonObject& request, const std::shared_ptr<Proxy>& proxy_client);
	static bool RemoveWhitelistUser(neb::CJsonObject& response, const neb::CJsonObject& request, const std::shared_ptr<Proxy>& proxy_client);
	static bool EnableWhitelist(neb::CJsonObject& response, const std::shared_ptr<Proxy>& proxy_client);
	static bool DisableWhitelist(neb::CJsonObject& response, const std::shared_ptr<Proxy>& proxy_client);
	static void GetUserProxyList(neb::CJsonObject& response, const std::shared_ptr<Proxy>& proxy_client);
	static bool SetUserProxy(neb::CJsonObject& response, const neb::CJsonObject& request, const std::shared_ptr<Proxy>& proxy_client);
	static bool RemoveUserProxy(neb::CJsonObject& response, const neb::CJsonObject& request, const std::shared_ptr<Proxy>& proxy_client);
	//��������������-1��ʾ������
	static bool SetMaxUsers(neb::CJsonObject& response, const neb::CJsonObject& request, const std::shared_ptr<Proxy>& proxy_client);
	static bool KickPlayer(neb::CJsonObject& response, const neb::CJsonObject& request, const std::shared_ptr<Proxy>& proxy_client);
	static void GetStartTime(neb::CJsonObject& response, const std::shared_ptr<Proxy>& proxy_client);
	 bool GetUserNumberList(neb::CJsonObject& response,neb::CJsonObject& request, const std::shared_ptr<Proxy>& proxy_client);
	void GetLogs(neb::CJsonObject& response, const std::shared_ptr<Proxy>& proxy_client);
public:
	WebControlServer(const std::string& address, std::uint16_t port);
	~WebControlServer() noexcept;

	void SetUserPassword(const std::string& password);
	//�ڶ����߳�������������
	void Start(std::shared_ptr<Proxy>& proxy_client);
	//ֹͣ���񲢵ȴ��������
	void Stop(void);

};