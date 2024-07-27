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

class Proxy {
protected:
	

public:
	/*让IDE可以提示，编写完成后放在protected*/
	std::list<RbsLib::Network::TCP::TCPConnection> connections;
	std::map<std::string, std::shared_ptr<User>> users;
	RbsLib::Network::TCP::TCPServer local_server;
	std::string remote_server_addr;
	std::uint16_t remote_server_port;
	bool is_ipv6_remote;
	std::shared_mutex global_mutex;
	RbsLib::Thread::TaskPool thread_pool = 11;
	/*END*/

	Proxy(bool is_ipv6_local, const std::string& local_address, std::uint16_t local_port, bool is_ipv6_remote, const std::string& remote_server_addr, std::uint16_t);
	Proxy(const Proxy&) = delete;
	Proxy& operator=(const Proxy&) = delete;
	void Start();
	void KickByUsername(const std::string& username);
	~Proxy() noexcept;
};
#endif // !PROXY_H
