# ���ο���
- ���ο�������ֱ���������еĴ�����ʵ���Լ���Ҫ���Զ��幦�ܣ��ܹ������������

## ����
- ���ο�����ָ�����еĴ������Ͻ��п�����ʵ���Լ���Ҫ���Զ��幦�ܡ����ο�������ͨ���������ṩ�Ľӿڽ��п�����Ҳ����ֱ���޸Ĵ����ܵ�Դ����п�����
- ���ο����������ǿ����������еĴ����ܣ�����ʵ���Լ���Ҫ�Ĺ��ܣ���߿���Ч�ʡ�
- Ҫʹ��MinecraftSpeedProxy���ж��ο���������Ҫ����ǰ�ֿ��¡�����������ñ�Ҫ��ͷ�ļ���ֱ��ʹ�����е���ͺ�����

## ��������
- ����������Ҫ��װCMake��g++��make�ȳ��õ�C++������������������Visual Studio Code��CLion��IDE�н��п�����

## ��������
- ��¡�ֿ�������
- ���ñ�Ҫ��ͷ�ļ�
- ʹ�ô������ṩ����ͺ������п���
- ��дCMakeLists.txt�ļ�
- ���ɲ�������Ŀ

����CMakeList.txt��ֱ���޸Ĳֿ��е�CMakeLists.txt�ļ�������Լ���Դ�ļ����ɡ�  
һ���򵥵������Ѿ��ڲֿ��е�example.cpp�ļ��и���������Բο�������ӽ��п�����
## ��ͺ���
### ::Proxy��
- Proxy���Ǵ����ܵĺ����࣬����ʵ�ִ����ܡ������ͨ��Proxy��Ľӿ�ʵ���Լ���Ҫ�Ĺ��ܡ�
#### ����
�������Ծ�Ҫ���ڷ�������ǰ���ã�������ܻᵼ��δ������Ϊ��

| ���� | ���� | ���� |
| --- | --- | --- |
|on_connected|RbsLib::Function::Function<void(const RbsLib::Network::TCP::TCPConnection& client)>|�κ�TCP���������������ʱִ�еĻص��������ϣ��׳��쳣�Ͽ����ӣ����԰���0��n���ص�������ͨ��+=��������µĻص�����������˳�������˳����ͬ|
|on_disconnected|RbsLib::Function::Function<void(const RbsLib::Network::TCP::TCPConnection& client)>|�κ�TCP���ӴӴ���������Ͽ�ʱִ�еĻص��������ϣ����԰���0��n���ص�������ͨ��+=��������µĻص�����������˳�������˳����ͬ|
|on_login|RbsLib::Function::Function<void(const RbsLib::Network::TCP::TCPConnection& client, const std::string& username, const std::string& uuid)>|�κ��û���¼ʱ���õĻص��������ϣ�����������������ͬ���׳��쳣��ܾ���¼�����԰���0��n���ص�������ͨ��+=��������µĻص�����������˳�������˳����ͬ|
|on_logout|RbsLib::Function::Function<void(const RbsLib::Network::TCP::TCPConnection& client, const UserInfo& userinfo)>|�κ��û��ǳ�ʱ���õĻص��������ϣ�����������������ͬ�����԰���0��n���ص�������ͨ��+=��������µĻص�����������˳�������˳����ͬ|

#### ����
| ���� | ���� |
| --- | --- |
|Proxy(bool is_ipv6_local, const std::string& local_address, std::uint16_t local_port, bool is_ipv6_remote, const std::string& remote_server_addr, std::uint16_t)|���캯���������еĵ�ַ����ΪIP������������Ҫע����Э��汾��Ӧ|
|void Start()|�������񡣸÷�������������������Ҫֹͣ�����������ö���|
|void KickByUsername(const std::string& username)|�����û����߳��û�|
|void KickByUUID(const std::string& uuid)|����UUID�߳��û�|
|auto GetUsersInfo() -> std::list\<UserInfo\>|��ȡ�����û���״̬��Ϣ|
|void SetMotd(const std::string& motd)|����MOTD,����Ϊmotd��JSON�ַ���|
|void SetMaxPlayers(std::uint16_t max_players)|������������|

### ::UserInfo��
- UserInfo�����ڴ洢�û�����Ϣ�������û�����UUID��IP��ַ���˿ڵ���Ϣ��һ����������һ�������û���״̬

### ::UserInfo::CallbackException��
- CallbackException�������ڻص��������׳��쳣���Ծܾ��û������ӻ��¼����
- ����̳���std::exception�����캯������һ��std::string���͵Ĳ��������û���½ʱ�׳����쳣������쳣ԭ����ʾ���û���Ϸ���沢�ܾ���¼

## ʾ������
������һ���򵥵�ʾ�����룬ʵ����һ���򵥵Ĵ�����������������û����ӡ��Ͽ�����¼���ǳ�ʱ���һЩ��Ϣ��
```cpp
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
```


