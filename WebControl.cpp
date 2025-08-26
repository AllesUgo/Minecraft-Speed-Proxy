#include "WebControl.h"
#include "logger.h"
#include <regex>
#include "json/CJsonObject.h"
#include "rbslib/CharsetConvert.h"
#include "WhiteBlackList.h"
#include <ranges>

asio::awaitable<void> WebControlServer::TimeTaskUsers(std::shared_ptr<Proxy>& proxy_client)
{
	try
	{
		asio::steady_timer timer(co_await asio::this_coro::executor);
		while (true) 
		{
			std::shared_lock<std::shared_mutex> lock(time_online_users_mutex);
			time_online_users[std::time(nullptr)] = proxy_client->GetUsersInfo().size();
			lock.unlock();
			timer.expires_after(std::chrono::minutes(1));
			co_await timer.async_wait(asio::use_awaitable);
		}
	}
	catch (...){}
}

void WebControlServer::SendErrorResponse(const RbsLib::Network::TCP::TCPConnection& connection, int status_code, const std::string& message)
{
	neb::CJsonObject json;
	json.Add("status", status_code);
	json.Add("message", RbsLib::Encoding::CharsetConvert::ANSItoUTF8(message));
	SendErrorResponse(connection, json);
}

void WebControlServer::SendErrorResponse(const RbsLib::Network::TCP::TCPConnection& connection, const neb::CJsonObject& json, int http_status_code)
{
	RbsLib::Network::HTTP::ResponseHeader response;
	response.status = http_status_code;
	response.headers.AddHeader("Content-Type", "application/json; charset=utf-8");
	response.headers.AddHeader("Content-Length", std::to_string(json.ToString().size()));
	response.headers.AddHeader("Access-Control-Allow-Origin", "*");
	RbsLib::Buffer response_buffer = response.ToBuffer();
	connection.Send(response.ToBuffer());
	connection.Send(json.ToString().c_str(), json.ToString().size());
}

void WebControlServer::SendSuccessResponse(const RbsLib::Network::TCP::TCPConnection& connection, const neb::CJsonObject& json)
{
	neb::CJsonObject wait_send_json = json;
	wait_send_json.Add("status", 200);
	wait_send_json.Add("message", "OK");
	auto json_str = wait_send_json.ToString();
	RbsLib::Network::HTTP::ResponseHeader response;
	response.status = 200;
	response.headers.AddHeader("Content-Type", "application/json; charset=utf-8");
	response.headers.AddHeader("Content-Length", std::to_string(json_str.size()));
	response.headers.AddHeader("Access-Control-Allow-Origin", "*");
	connection.Send(response.ToBuffer());
	connection.Send(json_str.c_str(), json_str.length());
}

bool WebControlServer::CheckToken(const std::string& cookie, const std::string& token, const std::chrono::system_clock::time_point& token_expiry_time)
{

	std::string extracted_token = cookie;
	return (extracted_token == token) && (std::chrono::system_clock::now() < token_expiry_time);
}

void WebControlServer::GetOnlineUsers(neb::CJsonObject& response, const std::shared_ptr<Proxy>& proxy_client)
{
	auto online_user_list = proxy_client->GetUsersInfo();
	auto proxy_map = proxy_client->GetUserProxyMap();
	auto default_proxy = proxy_client->GetDefaultProxy();
	response.AddEmptySubArray("online_users");
	for (const auto& user : online_user_list)
	{
		neb::CJsonObject user_info;
		user_info.Add("username", user.username);
		user_info.Add("uuid", user.uuid);
		user_info.Add("ip", user.ip);
		user_info.Add("current_proxy_flow", user.upload_bytes);
		user_info.Add("total_proxy_flow", 0);
		user_info.Add("online_time_stamp", user.connect_time);
		if (auto proxy_item = proxy_map.find(user.username); proxy_item != proxy_map.end())
		{
			user_info.Add("proxy_target", proxy_item->second.first + ":" + std::to_string(proxy_item->second.second));
		}
		else
		{
			user_info.Add("proxy_target", default_proxy.first + ":" + std::to_string(default_proxy.second));
		}
		response["online_users"].Add(user_info);
	}
}

void WebControlServer::GetWhiteList(neb::CJsonObject& response, const std::shared_ptr<Proxy>& proxy_client)
{
	auto white_list = WhiteBlackList::GetWhiteList();
	bool is_whitelist_on = WhiteBlackList::IsWhiteListOn();
	response.Add("whitelist_status", is_whitelist_on,is_whitelist_on);
	response.AddEmptySubArray("white_list");
	for (const auto& user : white_list)
	{
		response["white_list"].Add(user);
	}
}

void WebControlServer::GetBlackList(neb::CJsonObject& response, const std::shared_ptr<Proxy>& proxy_client)
{
	auto black_list = WhiteBlackList::GetBlackList();
	response.AddEmptySubArray("black_list");
	for (const auto& user : black_list)
	{
		response["black_list"].Add(user);
	}
}

bool WebControlServer::AddBlacklistUser(neb::CJsonObject& response, const neb::CJsonObject& request, const std::shared_ptr<Proxy>& proxy_client)
{
	std::string username;
	if (!request.Get("username",username))
		{
		response.Add("status", 400);
		response.Add("message", "Missing 'username' field in request");
		return false;
	}
	if (WhiteBlackList::IsInBlack(username))
	{
		response.Add("status", 400);
		response.Add("message", "User is already in black list");
		return false;
	}
	WhiteBlackList::AddBlackList(username);
	Logger::LogInfo("WebAPI: 已将用户 %s 添加到黑名单", username.c_str());
	response.Add("status", 200);
	response.Add("message", "User added to black list successfully");
	return true;
}

bool WebControlServer::RemoveBlacklistUser(neb::CJsonObject& response, const neb::CJsonObject& request, const std::shared_ptr<Proxy>& proxy_client)
{
	std::string username;
	if (!request.Get("username", username))
	{
		response.Add("status", 400);
		response.Add("message", "Missing 'username' field in request");
		return false;
	}
	if (!WhiteBlackList::IsInBlack(username))
	{
		response.Add("status", 400);
		response.Add("message", "User is not in black list");
		return false;
	}
	WhiteBlackList::RemoveBlackList(username);
	Logger::LogInfo("WebAPI: 已将用户 %s 从黑名单移除", username.c_str());
	response.Add("status", 200);
	response.Add("message", "User removed from black list successfully");
	return true;
}

bool WebControlServer::AddWhitelistUser(neb::CJsonObject& response, const neb::CJsonObject& request, const std::shared_ptr<Proxy>& proxy_client)
{
	std::string username;
	if (!request.Get("username", username))
	{
		response.Add("status", 400);
		response.Add("message", "Missing 'username' field in request");
		return false;
	}
	if (WhiteBlackList::IsInWhite(username))
	{
		response.Add("status", 400);
		response.Add("message", "User is already in white list");
		return false;
	}
	WhiteBlackList::AddWhiteList(username);
	Logger::LogInfo("WebAPI: 已将用户 %s 添加到白名单", username.c_str());
	response.Add("status", 200);
	response.Add("message", "User added to white list successfully");
	return true;
}

bool WebControlServer::RemoveWhitelistUser(neb::CJsonObject& response, const neb::CJsonObject& request, const std::shared_ptr<Proxy>& proxy_client)
{
	std::string username;
	if (!request.Get("username", username))
	{
		response.Add("status", 400);
		response.Add("message", "Missing 'username' field in request");
		return false;
	}
	if (!WhiteBlackList::IsInWhite(username))
	{
		response.Add("status", 400);
		response.Add("message", "User is not in white list");
		return false;
	}
	WhiteBlackList::RemoveWhiteList(username);
	Logger::LogInfo("WebAPI: 已将用户 %s 从白名单移除", username.c_str());
	response.Add("status", 200);
	response.Add("message", "User removed from white list successfully");
	return true;
}

bool WebControlServer::EnableWhitelist(neb::CJsonObject& response, const std::shared_ptr<Proxy>& proxy_client)
{
	if (WhiteBlackList::IsWhiteListOn())
	{
		response.Add("status", 400);
		response.Add("message", "Whitelist is already enabled");
		return false;
	}
	else
	{
		WhiteBlackList::WhiteListOn();
		Logger::LogInfo("WebAPI: 白名单已启用");
		response.Add("status", 200);
		response.Add("message", "Whitelist enabled successfully");
		return true;
	}
}

bool WebControlServer::DisableWhitelist(neb::CJsonObject& response, const std::shared_ptr<Proxy>& proxy_client)
{
	if (!WhiteBlackList::IsWhiteListOn())
	{
		response.Add("status", 400);
		response.Add("message", "Whitelist is already disabled");
		return false;
	}
	else
	{
		WhiteBlackList::WhiteListOff();
		Logger::LogInfo("WebAPI: 白名单已禁用");
		response.Add("status", 200);
		response.Add("message", "Whitelist disabled successfully");
		return true;
	}
}

void WebControlServer::GetUserProxyList(neb::CJsonObject& response, const std::shared_ptr<Proxy>& proxy_client)
{
	auto proxy_map = proxy_client->GetUserProxyMap();
	auto default_proxy = proxy_client->GetDefaultProxy();
	response.Add("default_proxy", default_proxy.first + ":" + std::to_string(default_proxy.second));
	response.AddEmptySubArray("user_proxies");
	for (const auto& item : proxy_map)
	{
		neb::CJsonObject user_proxy;
		user_proxy.Add("username", item.first);
		user_proxy.Add("proxy_target_addr", item.second.first);
		user_proxy.Add("proxy_target_port", item.second.second);
		response["user_proxies"].Add(user_proxy);
	}
}

bool WebControlServer::SetUserProxy(neb::CJsonObject& response, const neb::CJsonObject& request, const std::shared_ptr<Proxy>& proxy_client)
{
	std::string username;
	std::string proxy_address;
	int proxy_port;
	if (!request.Get("username", username) || !request.Get("proxy_address", proxy_address) || !request.Get("proxy_port", proxy_port))
	{
		response.Add("status", 400);
		response.Add("message", "Missing 'username', 'proxy_address' or 'proxy_port' field in request");
		return false;
	}
	if (proxy_port < 1 || proxy_port > 65535)
	{
		response.Add("status", 400);
		response.Add("message", "Invalid 'proxy_port' value, must be in range 1-65535");
		return false;
	}
	proxy_client->SetUserProxy(username, proxy_address, static_cast<std::uint16_t>(proxy_port));
	Logger::LogInfo("WebAPI: 已为用户 %s 设置代理服务器 %s:%d", username.c_str(), proxy_address.c_str(), proxy_port);
	response.Add("status", 200);
	response.Add("message", "User proxy set successfully");
	return true;
}

bool WebControlServer::RemoveUserProxy(neb::CJsonObject& response, const neb::CJsonObject& request, const std::shared_ptr<Proxy>& proxy_client)
{
	std::string username;
	if (!request.Get("username", username))
	{
		response.Add("status", 400);
		response.Add("message", "Missing 'username' field in request");
		return false;
	}
	try
	{
		proxy_client->DeleteUserProxy(username);
		Logger::LogInfo("WebAPI: 已删除用户 %s 的代理服务器设置", username.c_str());
	}
	catch (ProxyException const& ex)
	{
		response.Add("status", 400);
		response.Add("message", ex.what());
		return false;
	}
	response.Add("status", 200);
	response.Add("message", "User proxy removed successfully");
	return true;
}

bool WebControlServer::SetMaxUsers(neb::CJsonObject& response, const neb::CJsonObject& request, const std::shared_ptr<Proxy>& proxy_client)
{
	int max_users;
	if (!request.Get("max_users", max_users) || max_users < -1)
	{
		response.Add("status", 400);
		response.Add("message", "Invalid 'max_users' value");
		return false;
	}
	proxy_client->SetMaxPlayer(max_users);
	Logger::LogInfo("WebAPI: 已设置最大用户数为 %d", max_users);
	response.Add("status", 200);
	response.Add("message", "Max users set successfully");
	return true;
}

bool WebControlServer::KickPlayer(neb::CJsonObject& response, const neb::CJsonObject& request, const std::shared_ptr<Proxy>& proxy_client)
{
	std::string username;
	if (!request.Get("username", username))
	{
		response.Add("status", 400);
		response.Add("message", "Missing 'username' field in request");
		return false;
	}
	try
	{
		proxy_client->KickByUsername(username);
		Logger::LogInfo("WebAPI: 已踢出用户 %s", username.c_str());
		response.Add("status", 200);
		response.Add("message", "Player kicked successfully");
		return true;
	}
	catch (ProxyException const& ex)
	{
		response.Add("status", 400);
		response.Add("message", ex.what());
		return false;
	}
}

void WebControlServer::GetStartTime(neb::CJsonObject& response, const std::shared_ptr<Proxy>& proxy_client)
{
	auto start_time = proxy_client->GetStartTime();
	response.Add("start_time", start_time);
	response.Add("now_time", std::time(nullptr));
	response.Add("status", 200);
	response.Add("message", "Start time retrieved successfully");
}

// 修复后的 GetUserNumberList 相关代码
bool WebControlServer::GetUserNumberList(neb::CJsonObject& response, neb::CJsonObject& request, const std::shared_ptr<Proxy>& proxy_client)
{
	// 获取时间范围
	std::time_t start_time, end_time;
	if (!request.Get("start_time", start_time) || !request.Get("end_time", end_time) || start_time >= end_time)
	{
		response.Add("status", 400);
		response.Add("message", "Invalid 'start_time' or 'end_time' value");
		return false;
	}
	// 获取粒度 minute分钟、hour小时、day天、week周、month月
	std::string granularity;
	if (!request.Get("granularity", granularity) || (granularity != "minute" && granularity != "hour" && granularity != "day" && granularity != "week" && granularity != "month"))
	{
		response.Add("status", 400);
		response.Add("message", "Invalid 'granularity' value");
		return false;
	}
	// 利用ranges库获取时间范围内的在线用户数量
	std::shared_lock<std::shared_mutex> lock(this->time_online_users_mutex);
	auto filtered_view = std::ranges::views::filter(this->time_online_users, [start_time, end_time](const auto& item) {
		return item.first >= start_time && item.first <= end_time;
		});

	// 按照粒度分组统计
	std::map<std::time_t, uint32_t> grouped,count;
	for (const auto& item : filtered_view)
	{
		std::time_t key = 0;
		if (granularity == "minute")
		{
			key = item.first / 60 * 60;
		}
		else if (granularity == "hour")
		{
			key = item.first / 3600 * 3600;
		}
		else if (granularity == "day")
		{
			key = item.first / 86400 * 86400;
		}
		else if (granularity == "week")
		{
			key = item.first / (7 * 86400) * (7 * 86400);
		}
		else if (granularity == "month")
		{
			std::tm tm = *std::localtime(&item.first);
			tm.tm_mday = 1; // 设置为当月第一天
			tm.tm_hour = 0;
			tm.tm_min = 0;
			tm.tm_sec = 0;
			key = std::mktime(&tm);
		}
		grouped[key] += item.second;
		count[key]++; // 统计每个时间点的用户数量
	}
	// 计算每个时间点的平均在线用户数
	for (auto& item : grouped)
	{
		if (count[item.first] > 0)
		{
			item.second = static_cast<int>(std::round(static_cast<double>(item.second) / count[item.first])); // 计算平均值
		}
		else
		{
			item.second = 0; // 如果没有用户，则设置为0
		}
	}

	response.AddEmptySubArray("user_numbers");
	for (const auto& item : grouped)
	{
		neb::CJsonObject user_number;
		user_number.Add("timestamp", item.first);
		user_number.Add("online_users", item.second);
		response["user_numbers"].Add(user_number);
	}
	lock.unlock();
	response.Add("status", 200);
	response.Add("message", "User number list retrieved successfully");
	return true;
}

void WebControlServer::GetLogs(neb::CJsonObject& response, const std::shared_ptr<Proxy>& proxy_client)
{
	response.AddEmptySubArray("logs");
	std::shared_lock<std::shared_mutex> lock(this->log_mutex);
	for (const auto& log : this->logs)
	{
		neb::CJsonObject log_entry;
		log_entry.Add("timestamp", log.first);
		log_entry.Add("message", log.second);
		response["logs"].Add(log_entry);
	}
	lock.unlock();
	response.Add("status", 200);
	response.Add("message", "Logs retrieved successfully");
}

void WebControlServer::GetMotd(neb::CJsonObject& response, const std::shared_ptr<Proxy>& proxy_client)
{
	response.Add("motd", proxy_client->GetMotd());
	response.Add("status", 200);
	response.Add("message", "MOTD retrieved successfully");
}

bool WebControlServer::SetMotd(neb::CJsonObject& response, const neb::CJsonObject& request, const std::shared_ptr<Proxy>& proxy_client)
{
	neb::CJsonObject motd;
	if (request.Get("motd", motd))
	{
		proxy_client->SetMotd(motd.ToString());
		Logger::LogInfo("WebAPI: 已设置新的MOTD");
		response.Add("status", 200);
		response.Add("message", "MOTD set successfully");
		return true;
	}
	else
	{
		response.Add("status", 400);
		response.Add("message", "Missing or invalid 'motd' field in request");
		return false;
	}
}


WebControlServer::WebControlServer(const std::string& address, std::uint16_t port)
	:server(address, port)
{
}

WebControlServer::~WebControlServer() noexcept
{
	if (!this->is_request_stop)
	{
		this->Stop();
	}
}

void WebControlServer::SetUserPassword(const std::string& password)
{
	this->user_password = std::make_shared<std::string>(password);
}

void WebControlServer::Start(std::shared_ptr<Proxy>& proxy_client)
{
	//注册日志回调
	proxy_client->on_login += [this](Proxy::ConnectionControl& control) {
		std::unique_lock<std::shared_mutex> lock(this->log_mutex);
		this->logs.push_back({ std::time(nullptr), RbsLib::Encoding::CharsetConvert::ANSItoUTF8("玩家" + control.Username() + " uuid:" + control.UUID() + " 登录于 " + control.GetAddress()) });
		if (this->max_log_size > this->max_log_size)
		{
			this->logs.pop_front(); //删除最旧的日志
		}
		};
	proxy_client->on_logout += [this](Proxy::ConnectionControl& control) {
		double flow = control.UploadBytes();
		std::string unit = "bytes";
		if (flow > 10000) {
			flow /= 1024;
			unit = "KB";
		}
		if (flow > 10000) {
			flow /= 1024;
			unit = "MB";
		}
		if (flow > 10000) {
			flow /= 1024;
			unit = "GB";
		}
		double time = std::time(nullptr) - control.ConnectTime();
		std::string time_unit = "秒";
		if (time > 100) {
			time /= 60;
			time_unit = "分钟";
		}
		if (time > 100) {
			time /= 60;
			time_unit = "小时";
		}
		std::unique_lock<std::shared_mutex> lock(this->log_mutex);
		this->logs.push_back({ std::time(nullptr), RbsLib::Encoding::CharsetConvert::ANSItoUTF8("玩家" + control.Username() + " uuid:" + control.UUID() + " 退出于 " + control.GetAddress() + "，在线时长" + std::to_string(time) + time_unit + "，使用流量" + std::to_string(flow) + unit) });
		if (this->logs.size() > this->max_log_size)
		{
			this->logs.pop_front(); //删除最旧的日志
		}
		};
	this->server.SetGetHandle([proxy = proxy_client, this](const RbsLib::Network::TCP::TCPConnection& connection, RbsLib::Network::HTTP::RequestHeader& header) -> int {
		static const std::regex re_userproxy(R"(^/api/([a-zA-Z0-9_]{1,256})$)");
		std::cmatch m;
		if (std::regex_match(header.path.c_str(), m, re_userproxy) and m.size() == 2)
		{
			//检查token
			std::string cookie = header.headers.GetHeader("Authorize");
			if (!CheckToken(cookie, this->user_token, this->token_expiry_time))
			{
				WebControlServer::SendErrorResponse(connection, 401, "Unauthorized");
			}
			else
			{
				//根据业务逻辑处理请求
				
				neb::CJsonObject response_body;
				if (m[1].str() == "logout")
				{
					this->user_token = "";
					this->token_expiry_time = std::chrono::system_clock::time_point();
					Logger::LogInfo("WebAPI: 用户退出登录");
					response_body.Add("status", 200);
					response_body.Add("message", "Logout successful");
					RbsLib::Network::HTTP::ResponseHeader response_header;
					response_header.headers.AddHeader("Access-Control-Allow-Origin", "*");
					response_header.status = 200;
					response_header.headers.AddHeader("Content-Type", "application/json; charset=utf-8");
					response_header.headers.AddHeader("Content-Length", std::to_string(response_body.ToString().size()));
					connection.Send(response_header.ToBuffer());
					auto str = response_body.ToString();
					connection.Send(str.c_str(), str.size());
				}
				else if (m[1].str() == "get_online_users")
				{
					this->GetOnlineUsers(response_body, proxy);
					this->SendSuccessResponse(connection, response_body);
				}
				else if (m[1].str() == "get_whitelist")
				{
					this->GetWhiteList(response_body, proxy);
					this->SendSuccessResponse(connection, response_body);
				}
				else if (m[1].str() == "get_blacklist")
				{
					this->GetBlackList(response_body, proxy);
					this->SendSuccessResponse(connection, response_body);
				}
				else if (m[1].str() == "enable_whitelist")
				{
					if (this->EnableWhitelist(response_body, proxy))
					{
						this->SendSuccessResponse(connection, response_body);
					}
					else
					{
						this->SendErrorResponse(connection, response_body);
					}
				}
				else if (m[1].str() == "disable_whitelist")
				{
					if (this->DisableWhitelist(response_body, proxy))
					{
						this->SendSuccessResponse(connection, response_body);
					}
					else
					{
						this->SendErrorResponse(connection, response_body);
					}
				}
				else if (m[1].str() == "get_user_proxies")
				{
					this->GetUserProxyList(response_body, proxy);
					this->SendSuccessResponse(connection, response_body);
				}
				else if (m[1].str() == "get_start_time")
				{
					this->GetStartTime(response_body, proxy);
					this->SendSuccessResponse(connection, response_body);
				}
				else if (m[1].str() == "get_logs")
				{
					this->GetLogs(response_body, proxy);
					this->SendSuccessResponse(connection, response_body);
				}
				else if (m[1].str() == "get_motd")
				{
					this->GetMotd(response_body, proxy);
					this->SendSuccessResponse(connection, response_body);
				}
				else
				{
					WebControlServer::SendErrorResponse(connection, 404, "Unknown API endpoint");
				}
			}
		}
		else
		{
			WebControlServer::SendErrorResponse(connection, 404, "Unknown API endpoint");
		}
		return 0;
		});
	this->server.SetPostHandle([proxy = proxy_client, this](const RbsLib::Network::TCP::TCPConnection& connection, RbsLib::Network::HTTP::RequestHeader& header, RbsLib::Buffer& buffer) -> int {
		static const std::regex re_userproxy(R"(^/api/([a-zA-Z0-9_]{1,256})$)");
		std::cmatch m;
		if (std::regex_match(header.path.c_str(), m, re_userproxy) and m.size() == 2)
		{
			//解析JSON
			neb::CJsonObject data;
			if (!data.Parse(buffer.ToString()))
			{
				WebControlServer::SendErrorResponse(connection, 400, "Invalid JSON data");
				return 0;
			}
			if (m[1].str() == "login")
			{
				//登录请求
				std::string password = data("password");
				if (!this->user_password || *this->user_password != password)
				{
					WebControlServer::SendErrorResponse(connection, 401, "Invalid password");
					return 0;
				}
				//生成token
				this->user_token = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
				this->token_expiry_time = std::chrono::system_clock::now() + std::chrono::hours(1); // token有效期1小时
				Logger::LogInfo("WebAPI: 用户通过WebAPI登录成功");
				neb::CJsonObject response;
				response.Add("status", 200);
				response.Add("message", "Login successful");
				response.Add("token", this->user_token);
				response.Add("token_expiry_time", std::chrono::system_clock::to_time_t(this->token_expiry_time));
				RbsLib::Network::HTTP::ResponseHeader response_header;
				response_header.status = 200;
				response_header.headers.AddHeader("Content-Type", "application/json; charset=utf-8");
				response_header.headers.AddHeader("Access-Control-Allow-Origin", "*");
				response_header.headers.AddHeader("Content-Length", std::to_string(response.ToString().size()));
				connection.Send(response_header.ToBuffer());
				connection.Send(response.ToString().c_str(), response.ToString().size());
			}
			else
			{
				//检查token
				std::string cookie = header.headers.GetHeader("Authorize");
				if (!CheckToken(cookie, this->user_token, this->token_expiry_time))
				{
					WebControlServer::SendErrorResponse(connection, 401, "Unauthorized");
				}
				else
				{
					//根据业务逻辑处理请求
					neb::CJsonObject response_body;
					
					if (m[1].str() == "add_whitelist_user")
					{
						if (this->AddWhitelistUser(response_body, data, proxy))
						{
							this->SendSuccessResponse(connection, response_body);
						}
						else
						{
							WebControlServer::SendErrorResponse(connection, response_body);
						}
					}
					else if (m[1].str() == "remove_whitelist_user")
					{
						if (this->RemoveWhitelistUser(response_body, data, proxy))
						{
							this->SendSuccessResponse(connection, response_body);
						}
						else
						{
							WebControlServer::SendErrorResponse(connection, response_body);
						}
					}
					else if (m[1].str() == "add_blacklist_user")
					{
						if (this->AddBlacklistUser(response_body, data, proxy))
						{
							this->SendSuccessResponse(connection, response_body);
						}
						else
						{
							WebControlServer::SendErrorResponse(connection, response_body);
						}
					}
					else if (m[1].str() == "remove_blacklist_user")
					{
						if (this->RemoveBlacklistUser(response_body, data, proxy))
						{
							this->SendSuccessResponse(connection, response_body);
						}
						else
						{
							WebControlServer::SendErrorResponse(connection, response_body);
						}
					}
					else if (m[1].str() == "set_user_proxy")
					{
						if (this->SetUserProxy(response_body, data, proxy))
						{
							this->SendSuccessResponse(connection, response_body);
						}
						else
						{
							WebControlServer::SendErrorResponse(connection, response_body);
						}
					}
					else if (m[1].str() == "remove_user_proxy")
					{
						if (this->RemoveUserProxy(response_body, data, proxy))
						{
							this->SendSuccessResponse(connection, response_body);
						}
						else
						{
							WebControlServer::SendErrorResponse(connection, response_body);
						}
					}
					else if (m[1].str() == "set_max_users")
					{
						if (this->SetMaxUsers(response_body, data, proxy))
						{
							this->SendSuccessResponse(connection, response_body);
						}
						else
						{
							WebControlServer::SendErrorResponse(connection, response_body);
						}
					}
					else if (m[1].str() == "kick_player")
					{
						if (this->KickPlayer(response_body, data, proxy))
						{
							this->SendSuccessResponse(connection, response_body);
						}
						else
						{
							WebControlServer::SendErrorResponse(connection, response_body);
						}
					}
					else if (m[1].str() == "get_online_number_list")
					{
						if (this->GetUserNumberList(response_body, data, proxy))
						{
							this->SendSuccessResponse(connection, response_body);
						}
						else
						{
							WebControlServer::SendErrorResponse(connection, response_body);
						}
					}
					else if (m[1].str() == "set_motd")
					{
						if (this->SetMotd(response_body, data, proxy))
						{
							this->SendSuccessResponse(connection, response_body);
						}
						else
						{
							WebControlServer::SendErrorResponse(connection, response_body);
						}
					}
					else
					{
						WebControlServer::SendErrorResponse(connection, 404, "Unknown API endpoint");
					}
				}
			}
		}
		else
		{
			WebControlServer::SendErrorResponse(connection, 404, "Unknown API endpoint");
		}
		return 0;
		});

	this->server.SetOptionsHandle([this](const RbsLib::Network::TCP::TCPConnection& connection, RbsLib::Network::HTTP::RequestHeader& header) -> int {
		//处理OPTIONS请求，主要用于CORS预检请求
		RbsLib::Network::HTTP::ResponseHeader response;
		response.status = 204; // No Content
		response.headers.AddHeader("Access-Control-Allow-Origin", "*");
		response.headers.AddHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
		response.headers.AddHeader("Access-Control-Allow-Headers", "Content-Type, Authorize");
		response.headers.AddHeader("Access-Control-Max-Age", "86400"); // 24 hours
		connection.Send(response.ToBuffer());
		return 0;
		});
	std::thread([this]() {
		try
		{
			this->server.LoopWait(true, 3);
		}
		catch (const std::exception& e)
		{

		}
		}).detach();
	std::thread([this, &proxy_client]() {
		try
		{
			asio::co_spawn(this->io_context, this->TimeTaskUsers(proxy_client), asio::detached);
			this->io_context.run();
		}
		catch (const std::exception& e)
		{
		}
		}).detach();
}

void WebControlServer::Stop(void)
{
	this->is_request_stop = true;
	this->server.StopAndThrowExceptionInLoopThread();
	this->io_context.stop();
}
