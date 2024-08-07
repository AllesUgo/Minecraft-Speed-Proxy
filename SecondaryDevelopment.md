# 二次开发
- 二次开发可以直接利用现有的代理框架实现自己需要的自定义功能，能够满足各种需求。

## 概述
- 二次开发是指在现有的代理框架上进行开发，实现自己需要的自定义功能。二次开发可以通过代理框架提供的接口进行开发，也可以直接修改代理框架的源码进行开发。
- 二次开发的优势是可以利用现有的代理框架，快速实现自己需要的功能，提高开发效率。
- 要使用MinecraftSpeedProxy进行二次开发，你需要将当前仓库克隆至本机，引用必要的头文件并直接使用其中的类和函数。

## 开发环境
- 开发环境需要安装CMake、g++、make等常用的C++开发工具链，建议在Visual Studio Code、CLion等IDE中进行开发。

## 开发步骤
- 克隆仓库至本地
- 引用必要的头文件
- 使用代理框架提供的类和函数进行开发
- 编写CMakeLists.txt文件
- 生成并编译项目

其中CMakeList.txt可直接修改仓库中的CMakeLists.txt文件，添加自己的源文件即可。  
一个简单的例子已经在仓库中的example.cpp文件中给出，你可以参考这个例子进行开发。
## 类和函数
### ::Proxy类
- Proxy类是代理框架的核心类，用于实现代理功能。你可以通过Proxy类的接口实现自己需要的功能。
#### 属性
以下属性均要求在服务启动前设置，否则可能会导致未定义行为。

| 属性 | 类型 | 描述 |
| --- | --- | --- |
|on_connected|RbsLib::Function::Function<void(const RbsLib::Network::TCP::TCPConnection& client)>|任何TCP连接至代理服务器时执行的回调函数集合，抛出异常断开连接，可以包含0至n个回调函数，通过+=运算添加新的回调函数，调用顺序与添加顺序相同|
|on_disconnected|RbsLib::Function::Function<void(const RbsLib::Network::TCP::TCPConnection& client)>|任何TCP连接从代理服务器断开时执行的回调函数集合，可以包含0至n个回调函数，通过+=运算添加新的回调函数，调用顺序与添加顺序相同|
|on_login|RbsLib::Function::Function<void(const RbsLib::Network::TCP::TCPConnection& client, const std::string& username, const std::string& uuid)>|任何用户登录时调用的回调函数集合，参数含义与名称相同，抛出异常则拒绝登录，可以包含0至n个回调函数，通过+=运算添加新的回调函数，调用顺序与添加顺序相同|
|on_logout|RbsLib::Function::Function<void(const RbsLib::Network::TCP::TCPConnection& client, const UserInfo& userinfo)>|任何用户登出时调用的回调函数集合，参数含义与名称相同，可以包含0至n个回调函数，通过+=运算添加新的回调函数，调用顺序与添加顺序相同|

#### 方法
| 方法 | 描述 |
| --- | --- |
|Proxy(bool is_ipv6_local, const std::string& local_address, std::uint16_t local_port, bool is_ipv6_remote, const std::string& remote_server_addr, std::uint16_t)|构造函数，参数中的地址均可为IP或域名，但需要注意与协议版本对应|
|void Start()|启动服务。该方法不会阻塞服务，若需要停止服务，请析构该对象|
|void KickByUsername(const std::string& username)|按照用户名踢出用户|
|void KickByUUID(const std::string& uuid)|按照UUID踢出用户|
|auto GetUsersInfo() -> std::list\<UserInfo\>|获取所有用户的状态信息|
|void SetMotd(const std::string& motd)|设置MOTD,参数为motd的JSON字符串|
|void SetMaxPlayers(std::uint16_t max_players)|设置最大玩家数|

### ::UserInfo类
- UserInfo类用于存储用户的信息，包括用户名、UUID、IP地址、端口等信息。一般用于描述一个在线用户的状态

### ::UserInfo::CallbackException类
- CallbackException类用于在回调函数中抛出异常，以拒绝用户的连接或登录请求
- 该类继承自std::exception，构造函数包含一个std::string类型的参数，在用户登陆时抛出该异常将会把异常原因显示在用户游戏界面并拒绝登录

## 示例代码
下面是一个简单的示例代码，实现了一个简单的代理服务器，可以在用户连接、断开、登录、登出时输出一些信息。
```cpp
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
```


