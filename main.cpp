#include "rbslib/DataType.h"
#include "rbslib/Network.h"
#include <iostream>
#include <stdlib.h>
#include "proxy.h"
#include <list>

using namespace std;



int main()
{
	int is_ipv6_local;
	std::string local_address;
	std::uint16_t local_port;
	int is_ipv6_remote;
	std::string remote_server_addr;
	std::uint16_t remote_server_port;
	cout << "请输入本地是否为ipv6地址(0/1):";
	cin >> is_ipv6_local;
	cout << "请输入本地地址:";
	cin >> local_address;
	cout << "请输入本地端口:";
	cin >> local_port;
	cout << "请输入远程是否为ipv6地址(0/1):";
	cin >> is_ipv6_remote;
	cout << "请输入远程地址:";
	cin >> remote_server_addr;
	cout << "请输入远程端口:";
	cin >> remote_server_port;
	Proxy *proxy = new Proxy(is_ipv6_local, local_address, local_port, is_ipv6_remote, remote_server_addr, remote_server_port);
	proxy->on_connected+=[](const RbsLib::Network::TCP::TCPConnection& client) {
		std::cout <<client.GetAddress() <<"connected" << std::endl;
	};//注册连接回调
	proxy->on_login+= [](const RbsLib::Network::TCP::TCPConnection& client,const std::string& username, const std::string& uuid) {
		std::cout << "username:" << username << " uuid:" << uuid << " ip:" << client.GetAddress() << " login" << std::endl;
	};//注册登录回调
	proxy->on_logout+= [](const RbsLib::Network::TCP::TCPConnection& client, const UserInfo& userinfo) {
		std::cout << "username:" << userinfo.username << " uuid:" << userinfo.uuid << " ip:" << userinfo.ip << " logout" << std::endl;
	};//注册登出回调
	proxy->on_disconnect+= [](const RbsLib::Network::TCP::TCPConnection& client) {
		std::cout << client.GetAddress() << "disconnect" << std::endl;
	};//注册断开回调
	proxy->Start();
	std::string cmd;
	while (true)
	{
		cin >> cmd;
		if (cmd == "exit") {
			delete proxy;
			break;
		}
		else proxy->KickByUsername("AllesUgo");
	}
	return 0;
}