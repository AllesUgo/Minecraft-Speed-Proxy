#include "proxy.h"
#include <chrono>
#include <thread>

int main() {
	//����һ���������
	Proxy  *proxy = new Proxy(false,"0.0.0.0",25565,false,"mc.hypixel.net",25565);
	//���ô���Ļص�����
	//�����ص������Ĳ�����������鿴proxy.h������IDE��ת�����幦�ܲ鿴
	proxy->on_connected += [](const RbsLib::Network::TCP::TCPConnection& client) {
		std::cout << "Client connected: " << client.GetAddress() << std::endl;
	};
	proxy->on_disconnect += [](const RbsLib::Network::TCP::TCPConnection& client) {
		std::cout << "Client disconnected: " << client.GetAddress() << std::endl;
	};
	proxy->on_login += [](const RbsLib::Network::TCP::TCPConnection& client, const std::string& username, const std::string& uuid) {
		std::cout << "Client logged in: " << client.GetAddress() << " as " << username << " with UUID " << uuid << std::endl;
	};
	proxy->on_logout += [](const RbsLib::Network::TCP::TCPConnection& client,const UserInfo&user) {
		std::cout << "Client logged out: " << client.GetAddress() << " as " << user.username << " with UUID " << user.uuid << std::endl;
	};
	//��������������
	proxy->SetMaxPlayer(100);
	//��������
	proxy->Start();
	while (true) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	return 0;
}