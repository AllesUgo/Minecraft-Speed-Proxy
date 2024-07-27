#pragma once
#ifndef PROXY_H
#define PROXY_H
#include "rbslib/Network.h"
#include "rbslib/TaskPool.h"
#include "rbslib/Function.h"
#include <map>
#include "shared_mutex"

class ProxyException : public std::exception {
private: 
	std::string message;
	public:
		ProxyException(const std::string& message) noexcept;
		const char* what() const noexcept override;
};

class Proxy {
protected:
	

public:
	/*让IDE可以提示，编写完成后放在protected*/
	RbsLib::Network::TCP::TCPServer local_server;
	RbsLib::Thread::TaskPool task_pool;
	std::string remote_server_addr;
	std::uint16_t remote_server_port;
	bool is_ipv6_remote;
	std::shared_mutex users_connection_mutex;
	RbsLib::Thread::TaskPool thread_pool = 11;
	std::map<std::string, RbsLib::Network::TCP::TCPConnection> users_connection;
	/*END*/

	Proxy(bool is_ipv6_local, const std::string& local_address, std::uint16_t local_port, bool is_ipv6_remote, const std::string& remote_server_addr, std::uint16_t);
	Proxy(const Proxy&) = delete;
	Proxy& operator=(const Proxy&) = delete;
	void Start();
	void Stop();
};
#endif // !PROXY_H
