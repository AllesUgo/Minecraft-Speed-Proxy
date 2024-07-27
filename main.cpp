#include "rbslib/DataType.h"
#include "rbslib/Network.h"
#include <iostream>
#include <stdlib.h>
#include "proxy.h"
#include "proxy.h"

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
	proxy->Start();
	while (true)
	{
		getchar();
	}
	return 0;
}