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
#include <memory>
#include "rbslib/String.h"
#include <csignal>
#include "rbslib/BaseType.h"
#include "helper.h"
#include <semaphore>
#include "WebControl.h"

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

using namespace std;

std::shared_ptr<Proxy> proxy;

class ExitRequest :public std::exception {
public:
	int exit_code;
	ExitRequest(int code) :exit_code(code) {}
};

void PrintHelp() {
	cout << "Usage: " << endl;
	cout << "minecraftspeedproxy" << endl;
	cout << "\t<remote_server_address> <remote_server_port> <local_port>\tStart server" << endl;
	cout << "\t-h\tShow help" << endl;
	cout << "\t-c <config_file_path>\tStart with config file" << endl;
	cout << "\t-a <config_file_path>\tGenerate config file at specified location" << endl;
	cout << "\t--get-motd\tGet remote server's MOTD information" << endl;
	cout << "\t-v\tGet current version information" << endl;

	cout << "Current version config file is no longer compatible with versions prior to v3.0, please re-edit config file" << endl;
	
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
		cout << "Project URL: " << "https://github.com/AllesUgo/Minecraft-Speed-Proxy" << endl;
		exit(0);
	}
	else if (cmdline[1]=="-c") {
		if (cmdline.GetSize() != 3) {
			cout << "Parameter error, please use -h parameter to get help" << endl;
			exit(0);
		}
		Config::load_config(cmdline[2]);
	}
	else if (cmdline[1]=="-a") {
		if (cmdline.GetSize() != 3) {
			cout << "Parameter error, please use -h parameter to get help" << endl;
			exit(0);
		}
		Config::SetDefaultConfig();
		Config::save_config(cmdline[2]);
		exit(0);
	}
	else if (cmdline[1] == "--get-motd") {
		if (cmdline.GetSize() != 2) {
			cout << "Parameter error, please use -h parameter to get help" << endl;
			exit(0);
		}
		std::string addr;
		std::uint16_t port;
		std::cout << "This function helps to get remote server's MOTD information, including online player count, version number, image, etc." << std::endl;
		std::cout << "Please enter remote server address: ";
		std::cin >> addr;
		std::cout << "Please enter remote server port: ";
		std::cin >> port;
		try
		{
			auto result = Helper::GetRemoteServerMotd(addr, port);
			std::cout << "Do you want to save the result to a file? (y/n): ";
			std::string save;
			std::cin >> save;
			if (save == "y") {
				std::string path;
				std::cout << "Please enter save path: ";
				std::cin >> path;
				RbsLib::Storage::FileIO::File file_io(path,RbsLib::Storage::FileIO::OpenMode::Write|RbsLib::Storage::FileIO::OpenMode::Replace);
				file_io.Write(RbsLib::Buffer(result.ToFormattedString()));
				std::cout << "Saved to " << path << std::endl;
				std::cout << "Generally, please open the file with UTF-8 encoding" << std::endl;
			}
			else
			{
				std::cout << "The following is the raw JSON data formatted string of the remote server MOTD (if garbled, please save to file and open with UTF-8 encoding)" << std::endl;
				std::cout << result.ToFormattedString() << std::endl;
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "Error: " << e.what() << std::endl;
		}
		exit(0);
	}
	else {
		if (cmdline.GetSize() < 4) {
			cout << "Parameter error, please use -h parameter to get help" << endl;
			exit(0);
		}
		else {
			Config::SetDefaultConfig();
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
	executer.CreateSubOption("help", 0, "Show help", false, [](const RbsLib::Command::CommandExecuter::Args& args) {
		cout << "help: Show help" << endl;
		cout << "whitelist: Whitelist functions" << endl;
		cout << "ban <player_name> [...]: Ban users" << endl;
		cout << "pardon <player_name> [...]: Unban users" << endl;
		cout << "list: List players" << endl;
		cout << "kick <player_name> [...]: Kick users" << endl;
		cout << "motd: MOTD management" << endl;
		cout << "maxplayer [number]: Get/set maximum player count" << endl;
		cout << "userproxy <set|list>: User proxy server settings" << endl;
		cout << "ping: Test ping latency to target server" << endl;
		cout << "dnproxy: Domain name proxy functions" << endl;
		cout << "exit: Exit program" << endl;
		});
	executer.CreateSubOption("ping", 0, "Test ping latency to target server", false, [](const RbsLib::Command::CommandExecuter::Args& args) {
		if (proxy == nullptr) throw std::runtime_error("Service not started");
		try
		{
			Logger::LogInfo("Ping test latency: %dms", proxy->PingTest());
		}
		catch (const std::exception& e)
		{
			Logger::LogWarn("Ping test failed, please check remote server status: %s", e.what());
		}
		});
	executer.CreateSubOption("maxplayer", -1, "Set maximum player count", false, [](const RbsLib::Command::CommandExecuter::Args& args) {
		if (proxy == nullptr) throw std::runtime_error("Service not started");
		if (args.find("maxplayer") == args.end()) {
			cout << "Current online players: " << proxy->GetUsersInfo().size() << endl;
			cout << "Maximum players: " << proxy->GetMaxPlayer() << endl;
			return;
		}
		proxy->SetMaxPlayer(std::stoi(*args.find("maxplayer")->second.begin()));
		Logger::LogInfo("Set maximum player count to %d", std::stoi(*args.find("maxplayer")->second.begin()));
		});
	executer.CreateSubOption("motd", 0, "MOTD management", true);
	executer["motd"].CreateSubOption("reload", 0, "Reload MOTD", false, [](const RbsLib::Command::CommandExecuter::Args& args) {
		if (proxy == nullptr) throw std::runtime_error("Service not started");
		proxy->SetMotd(Motd::LoadMotdFromFile(Config::get_config<std::string>("MotdPath")));
		Logger::LogInfo("MOTD reloaded");
		});
	executer.CreateSubOption("kick", -1, "Kick users", false, [](const RbsLib::Command::CommandExecuter::Args& args) {
		if (args.find("kick") == args.end()) {
			cout << "Parameter error, please specify the player name to kick" << endl;
			return;
		}
		for (const auto& it : args.find("kick")->second) {
			if (proxy==nullptr) throw std::runtime_error("Service not started");
			try
			{
				proxy->KickByUsername(it);
				Logger::LogInfo("Kicked %s", it.c_str());
			}
			catch (const ProxyException& e)
			{
				Logger::LogWarn("User %s not found", it.c_str());
			}
			
		}
		});
	executer.CreateSubOption("exit", 0, "Exit program", false, [](const RbsLib::Command::CommandExecuter::Args& args) {
		Logger::LogInfo("Requesting exit");
		throw ExitRequest(0);
		});
	executer.CreateSubOption("whitelist", 0, "Whitelist functions", true, [](const RbsLib::Command::CommandExecuter::Args& args) {
		if (WhiteBlackList::IsWhiteListOn()) {
			cout << "Whitelist is enabled" << endl;
		}
		else {
			cout << "Whitelist is disabled" << endl;
		}
		cout<<"For more functions, enter whitelist -h"<<endl;
		});
	executer["whitelist"].CreateSubOption("on", 0, "Enable whitelist", false, [](const RbsLib::Command::CommandExecuter::Args& args) {
		if (WhiteBlackList::IsWhiteListOn()) {
			cout << "Whitelist is already enabled" << endl;
		}
		else {
			WhiteBlackList::WhiteListOn();
			cout << "Whitelist enabled" << endl;
		}
		});
	executer["whitelist"].CreateSubOption("off", 0, "Disable whitelist", false, [](const RbsLib::Command::CommandExecuter::Args& args) {
		if (!WhiteBlackList::IsWhiteListOn()) {
			cout << "Whitelist is already disabled" << endl;
		}
		else {
			WhiteBlackList::WhiteListOff();
			cout << "Whitelist disabled" << endl;
		}
		});
	executer["whitelist"].CreateSubOption("add", -1, "Add to whitelist", false,
		[](const RbsLib::Command::CommandExecuter::Args& args) {
			if (args.find("add") == args.end()) {
				cout << "Parameter error, please specify the player name to add to whitelist" << endl;
				return;
			}
			
			for (const auto& it : args.find("add")->second) {
				WhiteBlackList::AddWhiteList(it);
				cout << "Added " << it << " to whitelist" << endl;
			}
		});
	executer["whitelist"].CreateSubOption("remove", -1, "Remove from whitelist", false,
		[](const RbsLib::Command::CommandExecuter::Args& args) {
			if (args.find("remove") == args.end()) {
				cout << "Parameter error, please specify the player name to remove from whitelist" << endl;
				return;
			}
			for (const auto& it : args.find("remove")->second) {
				WhiteBlackList::RemoveWhiteList(it);
				cout << "Removed " << it << " from whitelist" << endl;
			}
		});
	executer.CreateSubOption("ban", -1, "Ban users (add to blacklist)", false,
		[](const RbsLib::Command::CommandExecuter::Args& args) {
			if (args.find("ban") == args.end()) {
				cout << "Parameter error, please specify the player name to add to blacklist" << endl;
				return;
			}
			for (const auto& it : args.find("ban")->second) {
				WhiteBlackList::AddBlackList(it);
				cout << "Added " << it << " to blacklist" << endl;
				//尝试踢出用户
				if (proxy != nullptr) {
					try
					{
						proxy->KickByUsername(it);
						Logger::LogInfo("Kicked %s", it.c_str());
					}
					catch (const ProxyException& e)
					{
					}
				}
			}
		});
	executer.CreateSubOption("pardon", -1, "Unban users", false,
		[](const RbsLib::Command::CommandExecuter::Args& args) {
			if (args.find("pardon") == args.end()) {
				cout << "Parameter error, please specify the player name to remove from blacklist" << endl;
				return;
			}
			for (const auto& it : args.find("pardon")->second) {
				WhiteBlackList::RemoveBlackList(it);
				cout << "Removed " << it << " from blacklist" << endl;
			}
		});
	executer.CreateSubOption("list", 0, "List players", true);
	executer["list"].CreateSubOption("whitelist", 0, "List whitelist", false, [](RbsLib::Command::CommandExecuter::Args const& args) {
		for (const auto& it : WhiteBlackList::GetWhiteList()) {
			cout << it << endl;
		}
		});
	executer["list"].CreateSubOption("blacklist", 0, "List blacklist", false, [](RbsLib::Command::CommandExecuter::Args const& args) {
		for (const auto& it : WhiteBlackList::GetBlackList()) {
			cout << it << endl;
		}
		});
	executer["list"].CreateSubOption("players", 0, "List online players", false, [](RbsLib::Command::CommandExecuter::Args const& args) {
		if (proxy == nullptr) throw std::runtime_error("Service not started");
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
	executer.CreateSubOption("userproxy", 0, "User proxy settings", true);
	executer["userproxy"].CreateSubOption("set", 3, "set <username> <address> <port>\tSet proxy server for specified user", false, [](const RbsLib::Command::CommandExecuter::Args& args) {
		if (args.find("set") == args.end() || args.find("set")->second.size() < 3) {
			cout << "Parameter error, please specify username, remote server address and port" << endl;
			return;
		}
		if (proxy == nullptr) throw std::runtime_error("Service not started");
		auto it = args.find("set")->second.begin();
		std::string username = *it++;
		std::string remote_server_address = *it++;
		std::uint16_t remote_server_port = std::stoi(*it);
		proxy->SetUserProxy(username, remote_server_address, remote_server_port);
		Logger::LogInfo("Set proxy server for %s to %s:%d", username.c_str(), remote_server_address.c_str(), remote_server_port);
		});
	executer["userproxy"].CreateSubOption("list", 0, "List all user proxy servers", false, [](const RbsLib::Command::CommandExecuter::Args& args) {
		if (proxy == nullptr) throw std::runtime_error("Service not started");
		auto user_proxy_map = proxy->GetUserProxyMap();
		if (user_proxy_map.empty()) {
			cout << "No user proxy servers set" << endl;
			return;
		}
		printf("%-15s %-36s\n", "username", "proxy");
		for (const auto& it : user_proxy_map) {
			printf("%-15s %-36s:%d\n", it.first.c_str(), it.second.first.c_str(), it.second.second);
		}
		});
	executer["userproxy"].CreateSubOption("delete", 1, "delete <username>\tDelete proxy server setting for specified user", false, [](const RbsLib::Command::CommandExecuter::Args& args) {
		if (args.find("delete") == args.end() || args.find("delete")->second.empty()) {
			cout << "Parameter error, please specify the username to delete proxy server setting" << endl;
			return;
		}
		if (proxy == nullptr) throw std::runtime_error("Service not started");
		for (const auto& it : args.find("delete")->second) {
			proxy->DeleteUserProxy(it);
			Logger::LogInfo("Deleted proxy server setting for %s", it.c_str());
		}
		});
	executer["userproxy"].CreateSubOption("clear", 0, "Clear all user proxy server settings", false, [](const RbsLib::Command::CommandExecuter::Args& args) {
		if (proxy == nullptr) throw std::runtime_error("Service not started");
		proxy->ClearUserProxy();
		Logger::LogInfo("Cleared all user proxy server settings");
		});
	executer.CreateSubOption("dnproxy", 0, "DNProxy settings", true);
	executer["dnproxy"].CreateSubOption("enable", 1, "Enable DNProxy", false, [](const RbsLib::Command::CommandExecuter::Args& args) {
		if (args.find("enable") == args.end() || args.find("enable")->second.empty()) {
			cout << "Parameter error, please specify the DNProxy server address" << endl;
			return;
		}
		proxy->EnableDomainNameProxy(*args.find("enable")->second.begin());
		Logger::LogInfo("Enabled DNProxy");
		});
	executer["dnproxy"].CreateSubOption("disable", 0, "Disable DNProxy", false, [](const RbsLib::Command::CommandExecuter::Args& args) {
		proxy->DisableDomainNameProxy();
		Logger::LogInfo("Disabled DNProxy");
		});

	executer["dnproxy"].CreateSubOption("add",3, "add <domain_name> <address> <port>\tAdd DNProxy domain name mapping", false, [](const RbsLib::Command::CommandExecuter::Args& args) {
		if (args.find("add") == args.end() || args.find("add")->second.size() != 3) {
			cout << "Parameter error, please specify domain name, address and port" << endl;
			return;
		}
		auto it = args.find("add")->second.begin();
		std::string domain_name = *it++;
		std::string address = *it++;
		std::uint16_t port = std::stoi(*it);
		proxy->AddDomainNameProxyMapping(domain_name, address, port);
		Logger::LogInfo("Added DNProxy mapping: %s -> %s:%d", domain_name.c_str(), address.c_str(), port);
		});
	executer["dnproxy"].CreateSubOption("remove", 1, "remove <domain_name>\tRemove DNProxy domain name mapping", false, [](const RbsLib::Command::CommandExecuter::Args& args) {
		if (args.find("remove") == args.end() || args.find("remove")->second.empty()) {
			cout << "Parameter error, please specify domain name to remove" << endl;
			return;
		}
		for (const auto& it : args.find("remove")->second) {
			proxy->RemoveDomainNameProxyMapping(it);
			Logger::LogInfo("Removed DNProxy mapping for domain name: %s", it.c_str());
		}
		});
	executer.Execute(argc, argv);
}


int main(int argc,const char**argv)
{
#ifdef _WIN32
	// 设置控制台编码为UTF-8
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
	
	// 设置C++流的locale为UTF-8
	std::locale::global(std::locale(""));
	std::cout.imbue(std::locale());
	std::cerr.imbue(std::locale());
	std::cin.imbue(std::locale());
#endif

#ifdef LINUX
	signal(SIGPIPE, SIG_IGN);
#endif // Linux

	try
	{
		MainCmdline(argc, argv);
		//初始化日志
		Logger::LogInfo("Initializing log service");
		if (Logger::Init(Config::get_config<std::string>("LogDir"), Config::get_config<int>("ShowLogLevel"), Config::get_config<int>("SaveLogLevel"))==false)
			Logger::LogError("Log initialization failed, unable to record logs");
		std::string local_address = Config::get_config<std::string>("LocalAddress");
		std::uint16_t local_port = Config::get_config<int>("LocalPort");
		std::string remote_server_addr = Config::get_config<std::string>("Address");
		std::uint16_t remote_server_port = Config::get_config<int>("RemotePort");
		bool enable_input = Config::get_config<bool>("AllowInput");
		Logger::LogInfo("Initializing whitelist and ban list");
		WhiteBlackList::Init();
		if (WhiteBlackList::IsWhiteListOn()) {
			Logger::LogInfo("Whitelist is enabled");
		}
		Logger::LogInfo("Local address: %s port: %d", local_address.c_str(), local_port);
		Logger::LogInfo("Remote server address: %s port: %d", remote_server_addr.c_str(), remote_server_port);
		proxy = std::make_shared<Proxy>(local_address, local_port, remote_server_addr, remote_server_port);
		/*
		proxy->on_connected += [](const RbsLib::Network::TCP::TCPConnection& client) {
			//std::cout <<client.GetAddress() <<"connected" << std::endl;
			};//注册连接回调
			*/

		proxy->on_login += [](Proxy::ConnectionControl& control) {
			if (WhiteBlackList::IsInBlack(control.Username()))
				control.isEnableConnect = false, control.reason = "You are in black list";
			else if (WhiteBlackList::IsWhiteListOn() && !WhiteBlackList::IsInWhite(control.Username()))
				control.isEnableConnect = false, control.reason = "You are not in white list";
			else
				Logger::LogPlayer("Player %s uuid:%s logged in from %s", control.Username().c_str(), control.UUID().c_str(), control.GetAddress().c_str());
			};
		proxy->on_logout += [](Proxy::ConnectionControl& control) {
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
			std::string time_unit = "seconds";
			if (time > 100) {
				time /= 60;
				time_unit = "minutes";
			}
			if (time > 100) {
				time /= 60;
				time_unit = "hours";
			}
			Logger::LogPlayer("Player %s uuid:%s logged out from %s, online duration %.1lf %s, traffic used %.3lf %s", control.Username().c_str(), control.UUID().c_str(), control.GetAddress().c_str(), time, time_unit.c_str(), flow, unit.c_str());
			};//注册登出回调
		//proxy->log_output += [](const char* str) {puts(str); };
		proxy->Start();
		proxy->SetMotd(Motd::LoadMotdFromFile(Config::get_config<std::string>("MotdPath")));
		proxy->SetMaxPlayer(Config::get_config<int>("MaxPlayer"));
		Logger::LogInfo("Testing ping to target server");
		try
		{
			Logger::LogInfo("Ping test latency: %dms", proxy->PingTest());
		}
		catch (const std::exception& e)
		{
			Logger::LogWarn("Ping test failed, please check remote server status: %s", e.what());
		}
		
		Logger::LogInfo("Service started");
		//检查是否开启命令行，如果不开启，则阻塞
		if (!enable_input)
		{
			Logger::LogInfo("Command line input is disabled, use CTRL-C to exit");
			counting_semaphore sem(0);
			sem.acquire(); //无限阻塞
		}
		//web启动
		std::shared_ptr<WebControlServer> web_server = nullptr;
		if (Config::get_config<bool>("WebAPIEnable"))
		{
			Logger::LogInfo("Starting WebAPI");
			std::string web_addr = Config::get_config<std::string>("WebAPIAddress");
			std::uint16_t web_port = Config::get_config<int>("WebAPIPort");
			std::string web_password = Config::get_config<std::string>("WebAPIPassword");
			if (web_addr.empty() || web_port == 0 || web_password.empty())
			{
				Logger::LogError("WebAPI configuration incomplete, please check config file");
				return 1;
			}
			Logger::LogInfo("WebAPI address: %s:%d", web_addr.c_str(), web_port);
			web_server = std::make_shared<WebControlServer>(web_addr, web_port);
			web_server->SetUserPassword(web_password);
			web_server->Start(proxy);
		}
		else
		{
			Logger::LogInfo("Web console is not enabled");
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
				std::thread([]() {
					this_thread::sleep_for(chrono::seconds(3));
					Logger::LogInfo("Server will exit within 5 seconds");
					this_thread::sleep_for(chrono::seconds(5));
					std::exit(0);
					}).detach();
				web_server = nullptr;
				proxy = nullptr;
				Logger::LogInfo("Server exited, exit code: %d", req.exit_code);
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
