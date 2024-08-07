#include "proxy.h"
#include <chrono>
#include <thread>

int main() {
	//创建一个代理对象
	Proxy  *proxy = new Proxy(false,"0.0.0.0",25565,false,"mc.hypixel.net",25565);
	//设置代理的回调函数
	//各个回调函数的参数及作用请查看proxy.h或利用IDE的转到定义功能查看
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
	//设置最大玩家数量
	proxy->SetMaxPlayer(100);
	//启动代理
	proxy->Start();
	while (true) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	return 0;
}