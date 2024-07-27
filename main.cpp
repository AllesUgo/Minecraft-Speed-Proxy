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
	cout << "�����뱾���Ƿ�Ϊipv6��ַ(0/1):";
	cin >> is_ipv6_local;
	cout << "�����뱾�ص�ַ:";
	cin >> local_address;
	cout << "�����뱾�ض˿�:";
	cin >> local_port;
	cout << "������Զ���Ƿ�Ϊipv6��ַ(0/1):";
	cin >> is_ipv6_remote;
	cout << "������Զ�̵�ַ:";
	cin >> remote_server_addr;
	cout << "������Զ�̶˿�:";
	cin >> remote_server_port;
	Proxy *proxy = new Proxy(is_ipv6_local, local_address, local_port, is_ipv6_remote, remote_server_addr, remote_server_port);
	proxy->on_connected+=[](const RbsLib::Network::TCP::TCPConnection& client) {
		std::cout <<client.GetAddress() <<"connected" << std::endl;
	};//ע�����ӻص�
	proxy->on_login+= [](const RbsLib::Network::TCP::TCPConnection& client,const std::string& username, const std::string& uuid) {
		std::cout << "username:" << username << " uuid:" << uuid << " ip:" << client.GetAddress() << " login" << std::endl;
	};//ע���¼�ص�
	proxy->on_logout+= [](const RbsLib::Network::TCP::TCPConnection& client, const UserInfo& userinfo) {
		std::cout << "username:" << userinfo.username << " uuid:" << userinfo.uuid << " ip:" << userinfo.ip << " logout" << std::endl;
	};//ע��ǳ��ص�
	proxy->on_disconnect+= [](const RbsLib::Network::TCP::TCPConnection& client) {
		std::cout << client.GetAddress() << "disconnect" << std::endl;
	};//ע��Ͽ��ص�
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