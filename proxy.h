#pragma once
#ifndef PROXY_H
#define PROXY_H
#include "rbslib/Network.h"
#include "rbslib/TaskPool.h"
#include "rbslib/Function.h"
#include <map>
#include <list>
#include <memory>
#include <shared_mutex>
#include <atomic>

class ProxyException : public std::exception {
private: 
	std::string message;
	public:
		ProxyException(const std::string& message) noexcept;
		const char* what() const noexcept override;
};
struct User {
	std::string username;
	std::string uuid;
	std::string ip;
	std::atomic_uint64_t upload_bytes = 0;
	std::time_t connect_time;
	RbsLib::Network::TCP::TCPConnection client, server;
	User(const RbsLib::Network::TCP::TCPConnection& client, const RbsLib::Network::TCP::TCPConnection& server);
};

/*�����û���Ϣ��������������Ϣ�ȣ����ڷ���*/
struct UserInfo {
	std::string username;
	std::string uuid;
	std::string ip;
	std::uint64_t upload_bytes;
	std::time_t connect_time;
};

class Proxy {

public:
	/*��IDE������ʾ����д��ɺ����protected*/
	
	/*END*/

	class CallbackException : public std::exception {
		//���������ڻص����׳����쳣�����쳣����ָʾ�ص���������״̬
	protected:
		std::string message;
	public:
		CallbackException(const std::string& message) noexcept;
		const char* what() const noexcept override;
	};

	//����Ϊ�ص�������������֤�̰߳�ȫ����Ҫע���̰߳�ȫ����
	RbsLib::Function::Function<void(const RbsLib::Network::TCP::TCPConnection& client)> on_connected;//���������ӳɹ��������Ӽ������ӳغ�,�׳��κ��쳣���������ӹر�
	RbsLib::Function::Function<void(const RbsLib::Network::TCP::TCPConnection& client)> on_disconnect;//�����ڼ�����ͻ��˶Ͽ�����ǰ����ʱ�����Ѳ����ã����������������շ����ݣ������ڱ�ʶ���ӡ����������շ��������쳣���׳��쳣��Ч
	RbsLib::Function::Function<void(const RbsLib::Network::TCP::TCPConnection& client, const std::string& username, const std::string& uuid)> on_login;//�������յ��ͻ��˵ĵ�¼���ݰ����û�δ���������û��б�login�׳����쳣������ʾ�ڿͻ���
	RbsLib::Function::Function<void(const RbsLib::Network::TCP::TCPConnection& client, const UserInfo& userinfo)> on_logout;//�������û������Ͽ�����֮ǰ���û��Ѵ������û��б����Ƴ���������δ�����ӳضϿ���logoutҪ����disconnectҪ��һ�£��ұ�֤logout����disconnect��logout�׳��쳣������δ������Ϊ

	Proxy(bool is_ipv6_local, const std::string& local_address, std::uint16_t local_port, bool is_ipv6_remote, const std::string& remote_server_addr, std::uint16_t);
	Proxy(const Proxy&) = delete;
	Proxy& operator=(const Proxy&) = delete;
	void Start();
	void KickByUsername(const std::string& username);
	void KickByUUID(const std::string& uuid);
	auto GetUsersInfo() -> std::list<UserInfo>;
	~Proxy() noexcept;
protected:

	std::list<RbsLib::Network::TCP::TCPConnection> connections;
	std::map<std::string, std::shared_ptr<User>> users;
	RbsLib::Network::TCP::TCPServer local_server;
	std::string remote_server_addr;
	std::uint16_t remote_server_port;
	bool is_ipv6_remote;
	std::shared_mutex global_mutex;
	RbsLib::Thread::TaskPool thread_pool = 11;
};
#endif // !PROXY_H
