﻿#include "rbslib/DataType.h"
#include "rbslib/Network.h"
#include <iostream>
#include <stdlib.h>
#include "proxy.h"
#include <list>
#include "logger.h"
#include "rbslib/Commandline.h"
#include "config.h"
#include "WhiteBlackList.h"
#include <memory>
#include "rbslib/String.h"
#include <csignal>
#include "rbslib/BaseType.h"
#include "helper.h"
#include <semaphore>

using namespace std;

std::unique_ptr<Proxy> proxy;

class ExitRequest :public std::exception {
public:
	int exit_code;
	ExitRequest(int code) :exit_code(code) {}
};

void PrintHelp() {
	cout << "Usage: " << endl;
	cout << "minecraftspeedproxy" << endl;
	cout << "\t<远程服务器地址> <远程服务器端口> <本地端口>\t启动服务器" << endl;
	cout << "\t-h\t显示帮助" << endl;
	cout << "\t-c <配置文件路径>\t通过配置文件启动" << endl;
	cout << "\t-a <配置文件路径>\t在指定位置生成配置文件" << endl;
	cout << "\t--get-motd\t获取远程服务器的Motd信息" << endl;
	cout << "\t-v\t获取当前版本信息" << endl;

	cout << "当前版本配置文件不再与v3.0以前兼容，请重新编辑配置文件" << endl;
	
}


void MainCmdline(int argc,const char** argv) {
	RbsLib::Command::CommandLine cmdline(argc, argv);
	if (cmdline[1]=="-h"||cmdline[1]=="--help") {
		PrintHelp();
		exit(0);
	}
	else if (cmdline[1]=="-v"||cmdline[1]=="--version") {
		cout << "MinecraftSpeedProxy ";
#ifdef RELEASE
		cout<< "Release ";
#endif
#ifdef DEBUG
		cout<< "Debug ";
#endif
		cout<< BUILD_VERSION << endl;
		cout << "项目地址: " << "https://github.com/AllesUgo/Minecraft-Speed-Proxy" << endl;
		exit(0);
	}
	else if (cmdline[1]=="-c") {
		if (cmdline.GetSize() != 3) {
			cout << "参数错误,请使用-h参数获取帮助" << endl;
			exit(0);
		}
		Config::load_config(cmdline[2]);
	}
	else if (cmdline[1]=="-a") {
		if (cmdline.GetSize() != 3) {
			cout << "参数错误,请使用-h参数获取帮助" << endl;
			exit(0);
		}
		Config::SetDeafultConfig();
		Config::save_config(cmdline[2]);
		exit(0);
	}
	else if (cmdline[1] == "--get-motd") {
		if (cmdline.GetSize() != 2) {
			cout << "参数错误,请使用-h参数获取帮助" << endl;
			exit(0);
		}
		std::string addr;
		std::uint16_t port;
		bool is_ipv6;
		std::cout << "此功能帮助获取远程服务器的Motd信息，包含在线玩家数、版本号、图片等信息" << std::endl;
		std::cout << "使用IPv6连接吗(一般服务器无需使用IPv6)？(y/n):";
		std::string ipv6;
		std::cin >> ipv6;
		if (ipv6 == "y") {
			is_ipv6 = true;
		}
		else {
			is_ipv6 = false;
		}
		std::cout << "请输入远程服务器地址:";
		std::cin >> addr;
		std::cout << "请输入远程服务器端口:";
		std::cin >> port;
		try
		{
			auto result = Helper::GetRemoteServerMotd(addr, port, is_ipv6);
			std::cout << "是否要将结果保存至文件中？(y/n) :";
			std::string save;
			std::cin >> save;
			if (save == "y") {
				std::string path;
				std::cout << "请输入保存路径:";
				std::cin >> path;
				RbsLib::Storage::FileIO::File file_io(path,RbsLib::Storage::FileIO::OpenMode::Write|RbsLib::Storage::FileIO::OpenMode::Replace);
				file_io.Write(RbsLib::Buffer(result.ToFormattedString()));
				std::cout << "已保存至" << path << std::endl;
				std::cout << "一般情况下请使用UTF-8编码打开文件" << std::endl;
			}
			else
			{
				std::cout << "以下为远程服务器MOTD的原始JSON数据格式化字符串(若存在乱码，请保存到文件并使用UTF-8编码打开)" << std::endl;
				std::cout << result.ToFormattedString() << std::endl;
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "错误:" << e.what() << std::endl;
		}
		exit(0);
	}
	else {
		if (cmdline.GetSize() < 4) {
			cout << "参数错误,请使用-h参数获取帮助" << endl;
			exit(0);
		}
		else {
			Config::SetDeafultConfig();
			Config::set_config("Address", cmdline[1]);
			Config::set_config("RemotePort",std::stoi(cmdline[2]));
			Config::set_config("LocalPort", std::stoi(cmdline[3]));
		}
	}
}

void InnerCmdline(int argc, const char** argv) {
	RbsLib::Command::CommandExecuter executer;
	executer.SetOutputCallback([](const std::string& str) {
		cout << str << endl;
		});
	executer.CreateSubOption("help", 0, "显示帮助", false, [](const RbsLib::Command::CommandExecuter::Args& args) {
		cout<<"help: 显示帮助"<<endl;
		cout<<"whitelist: 白名单功能"<<endl;
		cout<<"ban <player_name> [...]: 封禁用户"<<endl;
		cout<<"pardon <player_name> [...]: 取消封禁用户"<<endl;
		cout<<"list: 列出名单"<<endl;
		cout<<"kick <player_name> [...]: 踢出用户"<<endl;
		cout<<"motd: Motd管理"<<endl;
		cout<<"maxplayer <number>: 设置最大玩家数"<<endl;
		cout<<"ping: 测试与目标服务器的Ping延迟"<<endl;
		cout<<"exit: 退出程序"<<endl;
		});
	executer.CreateSubOption("ping", 0, "测试与目标服务器的Ping延迟", false, [](const RbsLib::Command::CommandExecuter::Args& args) {
		if (proxy == nullptr) throw std::runtime_error("服务未启动");
		try
		{
			Logger::LogInfo("测试Ping延迟：%dms", proxy->PingTest());
		}
		catch (const std::exception& e)
		{
			Logger::LogWarn("Ping测试失败，请检查远程服务器状态：%s", e.what());
		}
		});
	executer.CreateSubOption("maxplayer", 1, "设置最大玩家数", false, [](const RbsLib::Command::CommandExecuter::Args& args) {
		if (proxy == nullptr) throw std::runtime_error("服务未启动");
		if (args.find("maxplayer") == args.end()) {
			cout << "参数错误,请指定最大玩家数" << endl;
			return;
		}
		proxy->SetMaxPlayer(std::stoi(*args.find("maxplayer")->second.begin()));
		Logger::LogInfo("已设置最大玩家数为%d", std::stoi(*args.find("maxplayer")->second.begin()));
		});
	executer.CreateSubOption("motd", 0, "Motd管理", true);
	executer["motd"].CreateSubOption("reload", 0, "重新加载Motd", false, [](const RbsLib::Command::CommandExecuter::Args& args) {
		if (proxy == nullptr) throw std::runtime_error("服务未启动");
		proxy->SetMotd(Motd::LoadMotdFromFile(Config::get_config<std::string>("MotdPath")));
		Logger::LogInfo("已重新加载Motd");
		});
	executer.CreateSubOption("kick", -1, "踢出用户", false, [](const RbsLib::Command::CommandExecuter::Args& args) {
		if (args.find("kick") == args.end()) {
			cout << "参数错误,请指定要踢出的玩家名称" << endl;
			return;
		}
		for (const auto& it : args.find("kick")->second) {
			if (proxy==nullptr) throw std::runtime_error("服务未启动");
			try
			{
				proxy->KickByUsername(it);
				Logger::LogInfo("已踢出%s", it.c_str());
			}
			catch (const ProxyException& e)
			{
				Logger::LogWarn("未找到用户%s", it.c_str());
			}
			
		}
		});
	executer.CreateSubOption("exit", 0, "退出程序", false, [](const RbsLib::Command::CommandExecuter::Args& args) {
		Logger::LogInfo("正在请求退出");
		throw ExitRequest(0);
		});
	executer.CreateSubOption("whitelist", 0, "白名单功能", true, [](const RbsLib::Command::CommandExecuter::Args& args) {
		if (WhiteBlackList::IsWhiteListOn()) {
			cout << "白名单已启用" << endl;
		}
		else {
			cout << "白名单未启用" << endl;
		}
		cout<<"更多功能请输入whitelist -h"<<endl;
		});
	executer["whitelist"].CreateSubOption("on", 0, "启用白名单", false, [](const RbsLib::Command::CommandExecuter::Args& args) {
		if (WhiteBlackList::IsWhiteListOn()) {
			cout << "白名单已启用" << endl;
		}
		else {
			WhiteBlackList::WhiteListOn();
			cout << "已启用白名单" << endl;
		}
		});
	executer["whitelist"].CreateSubOption("off", 0, "关闭白名单", false, [](const RbsLib::Command::CommandExecuter::Args& args) {
		if (!WhiteBlackList::IsWhiteListOn()) {
			cout << "白名单已关闭" << endl;
		}
		else {
			WhiteBlackList::WhiteListOff();
			cout << "已关闭白名单" << endl;
		}
		});
	executer["whitelist"].CreateSubOption("add", -1, "添加名单", false,
		[](const RbsLib::Command::CommandExecuter::Args& args) {
			if (args.find("add") == args.end()) {
				cout << "参数错误,请指定要加入白名单的玩家名称" << endl;
				return;
			}
			
			for (const auto& it : args.find("add")->second) {
				WhiteBlackList::AddWhiteList(it);
				cout << "已添加" << it << "到白名单" << endl;
			}
		});
	executer["whitelist"].CreateSubOption("remove", -1, "移除名单", false,
		[](const RbsLib::Command::CommandExecuter::Args& args) {
			if (args.find("remove") == args.end()) {
				cout << "参数错误,请指定要移除白名单的玩家名称" << endl;
				return;
			}
			for (const auto& it : args.find("remove")->second) {
				WhiteBlackList::RemoveWhiteList(it);
				cout << "已移除" << it << "从白名单" << endl;
			}
		});
	executer.CreateSubOption("ban", -1, "封禁用户（加入黑名单）", false,
		[](const RbsLib::Command::CommandExecuter::Args& args) {
			if (args.find("ban") == args.end()) {
				cout << "参数错误,请指定要加入黑名单的玩家名称" << endl;
				return;
			}
			for (const auto& it : args.find("ban")->second) {
				WhiteBlackList::AddBlackList(it);
				cout << "已添加" << it << "到黑名单" << endl;
				//尝试踢出用户
				if (proxy != nullptr) {
					try
					{
						proxy->KickByUsername(it);
						Logger::LogInfo("已踢出%s", it.c_str());
					}
					catch (const ProxyException& e)
					{
					}
				}
			}
		});
	executer.CreateSubOption("pardon", -1, "取消封禁用户", false,
		[](const RbsLib::Command::CommandExecuter::Args& args) {
			if (args.find("pardon") == args.end()) {
				cout << "参数错误,请指定要移除黑名单的玩家名称" << endl;
				return;
			}
			for (const auto& it : args.find("pardon")->second) {
				WhiteBlackList::RemoveBlackList(it);
				cout << "已移除" << it << "从黑名单" << endl;
			}
		});
	executer.CreateSubOption("list", 0, "列出名单", true);
	executer["list"].CreateSubOption("whitelist", 0, "列出白名单", false, [](RbsLib::Command::CommandExecuter::Args const& args) {
		for (const auto& it : WhiteBlackList::GetWhiteList()) {
			cout << it << endl;
		}
		});
	executer["list"].CreateSubOption("blacklist", 0, "列出黑名单", false, [](RbsLib::Command::CommandExecuter::Args const& args) {
		for (const auto& it : WhiteBlackList::GetBlackList()) {
			cout << it << endl;
		}
		});
	executer["list"].CreateSubOption("players", 0, "列出在线玩家", false, [](RbsLib::Command::CommandExecuter::Args const& args) {
		if (proxy == nullptr) throw std::runtime_error("服务未启动");
		printf("%-15s %-36s %-19s %-10s %s\n", "username", "uuid", "login-time", "flow", "address");
		for (auto& it : proxy->GetUsersInfo()) {
			std::string unit = "bytes";
			if (it.upload_bytes > 10000) {
				it.upload_bytes /= 1024;
				unit = "KB";
			}
			if (it.upload_bytes > 10000) {
				it.upload_bytes /= 1024;
				unit = "MB";
			}
			if (it.upload_bytes > 10000) {
				it.upload_bytes /= 1024;
				unit = "GB";
			}
			std::string flow = RbsLib::String::Convert::ToString(it.upload_bytes,2)+' ' + unit;

			printf("%-15s %-36s %-19s %-10s %s\n", it.username.c_str(), it.uuid.c_str(), Time::ConvertTimeStampToFormattedTime(it.connect_time).c_str(), flow.c_str(), it.ip.c_str());
		}
		});
	executer.Execute(argc, argv);
}


int main(int argc,const char**argv)
{
#ifdef LINUX
	signal(SIGPIPE, SIG_IGN);
#endif // Linux

	try
	{
		MainCmdline(argc, argv);
		//初始化日志
		Logger::LogInfo("正在初始化日志服务");
		if (Logger::Init(Config::get_config<std::string>("LogDir"), Config::get_config<int>("ShowLogLevel"), Config::get_config<int>("SaveLogLevel"))==false)
			Logger::LogError("日志初始化失败，无法记录日志");
		int is_ipv6_local = Config::get_config<bool>("LocalIPv6");
		std::string local_address = Config::get_config<std::string>("LocalAddress");
		std::uint16_t local_port = Config::get_config<int>("LocalPort");
		int is_ipv6_remote = Config::get_config<bool>("RemoteIPv6");
		std::string remote_server_addr = Config::get_config<std::string>("Address");
		std::uint16_t remote_server_port = Config::get_config<int>("RemotePort");
		bool enable_input = Config::get_config<bool>("AllowInput");
		Logger::LogInfo("正在初始化白名单及封禁列表");
		WhiteBlackList::Init();
		if (WhiteBlackList::IsWhiteListOn()) {
			Logger::LogInfo("白名单已启用");
		}
		if (is_ipv6_local)
			Logger::LogInfo("本地地址为IPv6地址");
		else
			Logger::LogInfo("本地地址为IPv4地址");
		Logger::LogInfo("本地地址：%s 端口：%d", local_address.c_str(), local_port);
		if (is_ipv6_remote)
			Logger::LogInfo("远程服务器地址为IPv6地址");
		else
			Logger::LogInfo("远程服务器地址为IPv4地址");
		Logger::LogInfo("远程服务器地址：%s 端口：%d", remote_server_addr.c_str(), remote_server_port);
		proxy = std::make_unique<Proxy>(is_ipv6_local, local_address, local_port, is_ipv6_remote, remote_server_addr, remote_server_port);
		/*
		proxy->on_connected += [](const RbsLib::Network::TCP::TCPConnection& client) {
			//std::cout <<client.GetAddress() <<"connected" << std::endl;
			};//注册连接回调
			*/

		proxy->on_login += [](const RbsLib::Network::TCP::TCPConnection& client, const std::string& username, const std::string& uuid) {
			if (WhiteBlackList::IsInBlack(username))
				throw Proxy::CallbackException("You are in black list");
			};
		proxy->on_login += [](const RbsLib::Network::TCP::TCPConnection& client, const std::string& username, const std::string& uuid) {
			if (!WhiteBlackList::IsInWhite(username))
				throw Proxy::CallbackException("You are not in white list");
			};
		proxy->on_login += [](const RbsLib::Network::TCP::TCPConnection& client, const std::string& username, const std::string& uuid) {
			Logger::LogPlayer("玩家%s uuid:%s 登录于 %s", username.c_str(), uuid.c_str(), client.GetAddress().c_str());
			};//注册登录回调
		proxy->on_logout += [](const RbsLib::Network::TCP::TCPConnection& client, const UserInfo& userinfo) {
			double flow = userinfo.upload_bytes;
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
			double time = std::time(nullptr) - userinfo.connect_time;
			std::string time_unit = "秒";
			if (time > 100) {
				time /= 60;
				time_unit = "分钟";
			}
			if (time > 100) {
				time /= 60;
				time_unit = "小时";
			}
			Logger::LogPlayer("玩家%s uuid:%s 退出于 %s，在线时长%.1lf %s，使用流量%.3lf %s", userinfo.username.c_str(), userinfo.uuid.c_str(), client.GetAddress().c_str(), time, time_unit.c_str(), flow, unit.c_str());
			};//注册登出回调
		proxy->on_disconnect += [](const RbsLib::Network::TCP::TCPConnection& client) {
			//std::cout << client.GetAddress() << "disconnect" << std::endl;
			};//注册断开回调
		proxy->exception_handle += [](const std::exception& ex) {
			Logger::LogDebug("Exception: %s Reason: %s", typeid(ex).name(),ex.what());
			};//注册异常处理回调
		proxy->SetMotd(Motd::LoadMotdFromFile(Config::get_config<std::string>("MotdPath")));
		proxy->SetMaxPlayer(Config::get_config<int>("MaxPlayer"));
		Logger::LogInfo("正在测试与目标服务器的Ping");
		try
		{
			Logger::LogInfo("测试Ping延迟：%dms", proxy->PingTest());
		}
		catch (const std::exception& e)
		{
			Logger::LogWarn("Ping测试失败，请检查远程服务器状态：%s", e.what());
		}
		proxy->Start();
		Logger::LogInfo("服务已启动");
		//检查是否开启命令行，如果不开启，则阻塞
		if (!enable_input)
		{
			Logger::LogInfo("命令行输入已禁用，退出请使用CTRL-C");
			counting_semaphore sem(0);
			sem.acquire(); //无限阻塞
		}
		std::string cmd;
		while (true)
		{
			try
			{
				std::getline(std::cin, cmd);
				RbsLib::Command::CommandLine cmdline;
				cmdline.Parse(cmd);

				if (cmdline.GetSize() < 1)
				{
					throw std::runtime_error("Command line is empty");
				}
				//查找所需命令名称所在的模块
				int argc = cmdline.GetSize();
				std::unique_ptr<const char* []> argv(new const char* [argc]);
				try
				{
					for (int i = 0; i < argc; i++)
					{
						argv[i] = new char[cmdline[i].size() + 1];
						std::strcpy((char*)argv[i], cmdline[i].c_str());
					}
					InnerCmdline(argc, argv.get());
				}
				catch (...)
				{
					for (int i = 0; i < argc; ++i) delete[] argv[i];
					throw;
				}
				for (int i = 0; i < argc; ++i) delete[] argv[i];
			}
			catch (const ExitRequest& req)
			{
				proxy = nullptr;
				Logger::LogInfo("服务器已退出，退出码：%d", req.exit_code);
				return req.exit_code;
			}
			catch (const std::exception& e)
			{
				Logger::LogError("Error: %s", e.what());
			}
			catch (...)
			{
				Logger::LogError("Unknown error");
			}
		}
	}
	catch (const std::exception& e)
	{
		Logger::LogError("Error: %s", e.what());
	}
	catch (...)
	{
		Logger::LogError("Unknown error");
	}
	return 0;
}