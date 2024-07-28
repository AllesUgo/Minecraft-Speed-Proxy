#include "rbslib/DataType.h"
#include "rbslib/Network.h"
#include <iostream>
#include <stdlib.h>
#include "proxy.h"
#include <list>
#include "logger.h"
#include "rbslib/Commandline.h"
#include "config.h"
#include "WhiteBlackList.h"

using namespace std;

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
	cout<<"当前版本配置文件不再与v3.0以前兼容，请重新编辑配置文件"<<endl;
	
}


void MainCmdline(int argc,const char** argv) {
	RbsLib::Command::CommandLine cmdline(argc, argv);
	if (cmdline[1]=="-h") {
		PrintHelp();
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
		cout<<"ban: 封禁用户"<<endl;
		cout<<"pardon: 取消封禁用户"<<endl;
		cout<<"list: 列出名单"<<endl;
		cout<<"exit: 退出程序"<<endl;
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
	executer.Execute(argc, argv);
}


int main(int argc,const char**argv)
{
	MainCmdline(argc, argv);
	int is_ipv6_local = Config::get_config<bool>("LocalIPv6");
	std::string local_address = Config::get_config<std::string>("LocalAddress");
	std::uint16_t local_port = Config::get_config<int>("LocalPort");
	int is_ipv6_remote = Config::get_config<bool>("RemoteIPv6");
	std::string remote_server_addr = Config::get_config<std::string>("Address");
	std::uint16_t remote_server_port = Config::get_config<int>("RemotePort");
	WhiteBlackList::Init();
	if (WhiteBlackList::IsWhiteListOn()) {
		Logger::LogInfo("白名单已启用");
	}
	
	Proxy *proxy = new Proxy(is_ipv6_local, local_address, local_port, is_ipv6_remote, remote_server_addr, remote_server_port);
	proxy->on_connected+=[](const RbsLib::Network::TCP::TCPConnection& client) {
		//std::cout <<client.GetAddress() <<"connected" << std::endl;
	};//注册连接回调

	proxy->on_login += [](const RbsLib::Network::TCP::TCPConnection& client, const std::string& username, const std::string& uuid) {
		if (WhiteBlackList::IsInBlack(username))
			throw Proxy::CallbackException("You are in black list");
		};
	proxy->on_login += [](const RbsLib::Network::TCP::TCPConnection& client, const std::string& username, const std::string& uuid) {
		if (!WhiteBlackList::IsInWhite(username))
			throw Proxy::CallbackException("You are not in white list");
		};
	proxy->on_login+= [](const RbsLib::Network::TCP::TCPConnection& client,const std::string& username, const std::string& uuid) {
		Logger::LogInfo("玩家%s uuid:%s 登录于 %s\n", username.c_str(), uuid.c_str(),client.GetAddress().c_str());
	};//注册登录回调
	proxy->on_logout+= [](const RbsLib::Network::TCP::TCPConnection& client, const UserInfo& userinfo) {
		Logger::LogInfo("玩家%s uuid:%s 退出于 %s，使用流量%llu bytes\n", userinfo.username.c_str(), userinfo.uuid.c_str(), client.GetAddress().c_str(),userinfo.upload_bytes);
	};//注册登出回调
	proxy->on_disconnect+= [](const RbsLib::Network::TCP::TCPConnection& client) {
		//std::cout << client.GetAddress() << "disconnect" << std::endl;
	};//注册断开回调
	proxy->Start();
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
			delete proxy;
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
	return 0;
}