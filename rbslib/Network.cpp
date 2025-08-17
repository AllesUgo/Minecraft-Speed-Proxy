#include "Network.h"
#include "TaskPool.h"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <regex>
#include <sstream>
#include <thread>
namespace net = RbsLib::Network;

static std::string read_word(const char* &buffer, int max_len);
static std::string read_line(const char* &buffer, int max_len);
static int move_ptr_to_next_printable(const char*& ptr, int max_len);//返回值指示发生移动的距离，负数表示后方无可移动到的字符
static bool is_empty_line(const std::string& str);

static void close_socket(int sockfd); 

void close_socket(int sockfd)
{
#ifdef _WIN32
	closesocket(sockfd);
#else
	close(sockfd);
#endif
}

void RbsLib::Network::init_network()
{
#ifdef WIN32
	if (!net::network_inited)
	{
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
			throw net::NetworkException("Can not init winsock");
		else net::network_inited = true;
	}
#endif
}


RbsLib::Network::TCP::TCPServer::TCPServer(int port, const std::string& address)
{
	net::init_network();
	struct addrinfo hints, * res, * p;
	int opt;
	if (is_bind) throw net::NetworkException("This object is already bind");
	if (port < 0 || port>65535) throw net::NetworkException("Port mast be in range 0-65535");
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;    // 支持IPv4和IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP套接字
	hints.ai_flags = AI_PASSIVE;    // 用于绑定
	//若地址为空则绑定到所有地址
	if (getaddrinfo(address.empty()? NULL : address.c_str(), std::to_string(port).c_str(), &hints, &res) != 0) {
		throw net::NetworkException("Get address info failed");
	}

	for (p = res; p != NULL; p = p->ai_next) {
		this->server_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (this->server_socket == INVALID_SOCKET) continue;

		// 设置SO_REUSEADDR选项
		int yes = 1;
		if (setsockopt(this->server_socket, SOL_SOCKET, SO_REUSEADDR, (char*) & yes, sizeof(yes)) == -1) {
			freeaddrinfo(res);
			std::string reason = "setsockopt failed";
#ifdef LINUX
			reason += ": ";
			reason += strerror(errno);
#endif // LINUX
			close_socket(this->server_socket);
			throw net::NetworkException(reason);
		}
		yes = 0;
		if (p->ai_family == AF_INET6)
		{
			// IPv6允许IPv4连接
			if (setsockopt(this->server_socket, IPPROTO_IPV6, IPV6_V6ONLY, (char*) & yes, sizeof(yes)))
			{
				freeaddrinfo(res);
				std::string reason = "setsockopt IPV6_V6ONLY failed";
#ifdef LINUX
				reason += ": ";
				reason += strerror(errno);
#endif // LINUX
#ifdef WIN32
				reason += "; code: ";
				reason += std::to_string(WSAGetLastError());
#endif // WIN32
				close_socket(this->server_socket);
				throw net::NetworkException(reason);
			}
		}

		if (bind(this->server_socket, p->ai_addr, p->ai_addrlen))
		{
			freeaddrinfo(res);
			std::string reason = "Bind failed";
#ifdef WIN32
			reason += ", ErrorCode: ";
			reason += std::to_string(WSAGetLastError());
#endif // WIN32
#ifdef LINUX
			reason += ": ";
			reason += strerror(errno);
#endif // LINUX
			close_socket(this->server_socket);
			throw net::NetworkException(reason);
		}
		break;
	}
	freeaddrinfo(res);
	if (p == NULL)
	{
		throw net::NetworkException("Bind failed: No suitable address found");
	}

	this->reference_counter = new int;
	*this->reference_counter = 1;
}

RbsLib::Network::TCP::TCPServer::TCPServer(const TCPServer& server) noexcept
{
	this->server_socket = server.server_socket;
	this->is_bind = server.is_bind;
	this->is_listen = server.is_listen;
	this->reference_counter = server.reference_counter;
	if (reference_counter) ++*this->reference_counter;
}

RbsLib::Network::TCP::TCPServer::~TCPServer(void) noexcept
{
	this->Close();
}

const RbsLib::Network::TCP::TCPServer& RbsLib::Network::TCP::TCPServer::operator=(const TCPServer& server) noexcept
{
	if (this == &server) return *this;
	this->Close();
	this->server_socket = server.server_socket;
	this->is_bind = server.is_bind;
	this->is_listen = server.is_listen;
	this->reference_counter = server.reference_counter;
	if (reference_counter) ++*this->reference_counter;
	return *this;
}

SOCKET RbsLib::Network::TCP::TCPServer::GetSocket(void) const noexcept
{
	return this->server_socket;
}

void RbsLib::Network::TCP::TCPServer::Listen(int listen_num)
{
	if (!this->is_listen)
	{
		if (listen(this->server_socket, listen_num) == SOCKET_ERROR)
		{
			throw net::NetworkException("Start listening mode failed");
		}
		else this->is_listen = true;
	}
}

RbsLib::Network::TCP::TCPConnection RbsLib::Network::TCP::TCPServer::Accept(void)
{
	struct sockaddr_storage info = { 0 };
	SOCKET sock;
	socklen_t info_len = sizeof(info);
	if (!this->is_listen)
	{
		if (listen(this->server_socket, 5) == SOCKET_ERROR)
		{
			throw net::NetworkException("Start listening mode failed");
		}
		else this->is_listen = true;
	}
	if ((sock = accept(this->server_socket, (struct sockaddr*)&info, &info_len)) == INVALID_SOCKET)
	{
		throw net::NetworkException("Accept connection failed");
	}
	return RbsLib::Network::TCP::TCPConnection(sock, info, info_len);
}

void RbsLib::Network::TCP::TCPServer::Close(void) noexcept
{
	if (this->reference_counter)
	{
		--*this->reference_counter;
		if (*this->reference_counter == 0)
		{
#ifdef WIN32
			if (!this->is_force_closed) closesocket(this->server_socket);
#endif //Windows
#ifdef LINUX
			if (!this->is_force_closed) close(this->server_socket);
#endif // linux
			delete this->reference_counter;
			this->reference_counter = nullptr;
		}
	}
}

void RbsLib::Network::TCP::TCPServer::ForceClose(void)
{
	#ifdef WIN32
	shutdown(this->server_socket, SD_BOTH);
	closesocket(this->server_socket);
	#endif
	#ifdef LINUX
	shutdown(this->server_socket, SHUT_RDWR);
	close(this->server_socket);
	#endif
	this->is_force_closed = true;
}


RbsLib::Network::TCP::TCPConnection::TCPConnection(SOCKET sock, const struct sockaddr_storage& connection_info, int info_len) noexcept
{
	this->mutex = new std::mutex;
	this->sock = sock;
	this->connection_info = connection_info;
	this->connection_info_len = info_len;
	this->reference_counter = new int;
	*this->reference_counter = 1;
}


RbsLib::Network::TCP::TCPConnection::TCPConnection(const TCPConnection& connection) noexcept
{
	connection.mutex->lock();
	this->sock = connection.sock;
	this->reference_counter = connection.reference_counter;
	this->connection_info = connection.connection_info;
	this->connection_info_len = connection.connection_info_len;
	this->mutex = connection.mutex;
	if (this->reference_counter) ++*this->reference_counter;
	connection.mutex->unlock();
}

RbsLib::Network::TCP::TCPConnection::TCPConnection(TCPConnection&& connection) noexcept
{
	connection.mutex->lock();
	this->sock = connection.sock;
	this->reference_counter = connection.reference_counter;
	this->connection_info = connection.connection_info;
	this->mutex = connection.mutex;
	connection.reference_counter = nullptr;
	connection.mutex = nullptr;
	this->mutex->unlock();
}

RbsLib::Network::TCP::TCPConnection::~TCPConnection(void) noexcept
{
	this->Close();
}

const RbsLib::Network::TCP::TCPConnection& RbsLib::Network::TCP::TCPConnection::operator=(const TCPConnection& connection) noexcept
{
	if (this == &connection) return *this;
	this->Close();
	connection.mutex->lock();
	this->mutex = connection.mutex;
	this->sock = connection.sock;
	this->reference_counter = connection.reference_counter;
	this->connection_info = connection.connection_info;
	this->connection_info_len = connection.connection_info_len;
	*this->reference_counter += 1;
	connection.mutex->unlock();
	return *this;
}

const RbsLib::Network::TCP::TCPConnection& RbsLib::Network::TCP::TCPConnection::operator=(TCPConnection&& connection) noexcept
{
	if (this == &connection) return *this;
	this->Close();
	connection.mutex->lock();
	this->mutex = connection.mutex;
	this->sock = connection.sock;
	this->reference_counter = connection.reference_counter;
	this->connection_info = connection.connection_info;
	this->connection_info_len = connection.connection_info_len;
	connection.reference_counter = nullptr;
	connection.mutex->unlock();
	connection.mutex = nullptr;
	return *this;
}

bool RbsLib::Network::TCP::TCPConnection::operator==(const TCPConnection& connection) const noexcept
{
	return this->sock == connection.sock;
}

void RbsLib::Network::TCP::TCPConnection::Send(const RbsLib::IBuffer& buffer, int flag) const
{
	if (buffer.GetLength() > 0)
	{
		if (send(this->sock, (const char*)buffer.Data(), buffer.GetLength(), flag) != buffer.GetLength())
		{
			throw net::NetworkException("Send failed");
		}
	}
}

int RbsLib::Network::TCP::TCPConnection::Send(const void* data, int len, int flag) const noexcept
{
	return send(this->sock, (const char*)data, len, flag);
}

RbsLib::Buffer RbsLib::Network::TCP::TCPConnection::Recv(int len, int flag) const
{
	char* data = new char[len];
	int s;

	if ((s = recv(this->sock, data, len, flag)) <= 0)
	{
		delete[]data;
		throw net::NetworkException("Recv failed");
	}
	RbsLib::Buffer buffer(data, s);
	delete[] data;
	return buffer;
}

int RbsLib::Network::TCP::TCPConnection::Recv(RbsLib::Buffer& buffer, int flag) const
{
	char* data = new char[buffer.GetSize()];
	int s;
	if ((s = recv(this->sock, data, buffer.GetSize(), flag)) <= 0)
	{
		delete[]data;
		throw net::NetworkException("Recv failed");
	}
	buffer.Data(data, s);
	delete[]data;
	return s;
}

int RbsLib::Network::TCP::TCPConnection::Recv(void* data, int len, int flag) const noexcept
{
	return recv(this->sock, (char*)data, len, flag);
}

SOCKET RbsLib::Network::TCP::TCPConnection::GetSocket(void) const noexcept
{
	return this->sock;
}

SOCKET RbsLib::Network::TCP::TCPConnection::GetRowSocket(void) const noexcept
{
	return this->sock;
}

std::string RbsLib::Network::TCP::TCPConnection::GetAddress(void) const noexcept
{
	char buffer[INET6_ADDRSTRLEN];
	const void* src_addr = nullptr;

	if (this->connection_info.ss_family == AF_INET) {
		const sockaddr_in* sa = reinterpret_cast<const sockaddr_in*>(&this->connection_info);
		src_addr = &(sa->sin_addr);
	}
	else if (this->connection_info.ss_family == AF_INET6) {
		const sockaddr_in6* sa = reinterpret_cast<const sockaddr_in6*>(&this->connection_info);
		src_addr = &(sa->sin6_addr);
	}
	else {
		return "";  // 不是IPv4/IPv6地址
	}

	return inet_ntop(this->connection_info.ss_family,
		src_addr,
		buffer,
		sizeof(buffer))
		? std::string(buffer)
		: "";
}

void RbsLib::Network::TCP::TCPConnection::Close(void)
{
	if (this->mutex)
	{
		this->mutex->lock();
		if (this->reference_counter)
		{
			*this->reference_counter -= 1;
			if (*this->reference_counter <= 0)
			{
				/*引用计数器小于0，需要关闭并释放*/
#ifdef WIN32
				closesocket(this->sock);
#endif // WIN32
#ifdef LINUX
				close(this->sock);
#endif // linux
				delete this->reference_counter;
				this->reference_counter = nullptr;
				this->mutex->unlock();
				delete this->mutex;
			}
			else
			{
				/*引用计数大于0*/
				this->reference_counter = nullptr;
				this->mutex->unlock();
				this->mutex = nullptr;
			}
		}
		else
		{
			this->mutex->unlock();//未启用引用计数器，直接解锁
		}
	}
	/*如果Mutex为null，则该对象一定发生过移动构造、移动拷贝或已经释放，无需释放任何资源*/
}

void RbsLib::Network::TCP::TCPConnection::SetSocketOption(int level, int optname, const void* optval, socklen_t optlen) const
{
	if (setsockopt(this->sock, level, optname, (const char*)optval, optlen) == SOCKET_ERROR)
	{
		throw net::NetworkException("Set socket option failed");
	}
}

void RbsLib::Network::TCP::TCPConnection::Disable(void) const
{
	#ifdef WIN32
	shutdown(this->sock, SD_BOTH);
	#endif
	#ifdef LINUX
	shutdown(this->sock, SHUT_RDWR);
	#endif
}

void RbsLib::Network::TCP::TCPServer::Disable(void) const
{
#ifdef WIN32
	shutdown(this->server_socket, SD_BOTH);
#endif
#ifdef LINUX
	shutdown(this->server_socket, SHUT_RDWR);
#endif
}

RbsLib::Network::TCP::TCPConnection RbsLib::Network::TCP::TCPClient::Connect(std::string address, int port)
{
	net::init_network();
	struct addrinfo* answer = nullptr, hint = { 0 };
	hint.ai_family = AF_UNSPEC;  // 自动选择IPv4或IPv6
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_protocol = IPPROTO_TCP;
	if (getaddrinfo(address.c_str(), std::to_string(port).c_str(), &hint, &answer)) {
		throw net::NetworkException("Get address info failed");
	}
	SOCKET c_Socket = socket(answer->ai_family, answer->ai_socktype, answer->ai_protocol);
	if (SOCKET_ERROR == c_Socket)
	{
		freeaddrinfo(answer);
		throw net::NetworkException("Allocate socket failed");
	}
	if (connect(c_Socket, answer->ai_addr, answer->ai_addrlen))
	{
		freeaddrinfo(answer);
		throw net::NetworkException("Connect server failed");
	}
	struct sockaddr_storage addr = { 0 };

	memcpy(&addr, answer->ai_addr,answer->ai_addrlen);
	freeaddrinfo(answer);
	return net::TCP::TCPConnection(c_Socket, addr, sizeof(addr));
}




static std::string read_word(const char* &buffer, int max_len)
{
	std::string str;
	bool is_start = false;
	int i;
	for (i = 0; i < max_len && buffer[i]; ++i)
	{
		if (std::isalnum(buffer[i])||std::ispunct(buffer[i]))
		{
			is_start = true;
			str.push_back(buffer[i]);
		}
		else if (is_start) break;
	}
	buffer += i;
	return str;
}

static std::string read_line(const char* &buffer, int max_len)
{
	std::string line;
	int i;
	for (i = 0; i < max_len &&buffer[i] && buffer[i] != '\r' && buffer[i] != '\n'; ++i)
	{
		line.push_back(buffer[i]);
	}
	if (max_len - i >= 2)
	{
		if (buffer[i] == '\r' && buffer[i + 1] == '\n') line += "\r\n",i+=2;
		else if (buffer[i] == '\n') line.push_back('\n'),++i;
	}
	else if (max_len - i >= 1)
	{
		if (buffer[i] == '\n') line.push_back('\n'),++i;
		else if (buffer[i] == '\r') line.push_back('\r'),++i;
	}
	buffer += i;
	return line;
}

int move_ptr_to_next_printable(const char*& ptr, int max_len)
{
	int i;
	for (i = 0; i < max_len && ptr[i]; ++i)
	{
		if (std::isalnum(ptr[i])||std::ispunct(ptr[i]))
		{
			ptr += i;
			return i;
		}
	}
	return -1;
}

bool is_empty_line(const std::string& str)
{
	if (str.empty()) return true;
	else if (str == "\r\n") return true;
	else if (str == "\n") return true;
	return false;
}

RbsLib::Network::HTTP::HTTPServer::HTTPServer(const std::string& host, int port)
	:server(port, host)
{
}

RbsLib::Network::HTTP::HTTPServer::HTTPServer(int port)
	:server(port, "0.0.0.0")
{
}

void RbsLib::Network::HTTP::HTTPServer::LoopWait(bool use_thread_pool, int keep_threads_number)
{
	if (use_thread_pool)
	{
		//创建线程池
		RbsLib::Thread::TaskPool pool(keep_threads_number);
		while (true)
		{
			auto connection = this->server.Accept();
			std::string& protocol_version = this->protocol_version;
			auto& get = this->on_get_request;
			auto& post = this->on_post_request;
			auto& options = this->on_options_request;
			pool.Run([connection, protocol_version, get, post,options]() {
				//读取Header
				bool is_keep_alive = true;
				try
				{
					while (true)
					{
						RequestHeader header;
						const char* p;
						auto buffer = connection.Recv(1024 * 1024);
						const char* now_ptr = (const char*)buffer.Data();
						const char* end_ptr = (const char*)buffer.Data() + buffer.GetLength();
						//读取协议行
						std::string line = read_line(now_ptr, (int)(end_ptr - now_ptr));
						if (is_empty_line(line)) return;
						p = line.c_str();
						std::string method = read_word(p, (int)line.length());
						if (method == "GET") header.request_method = Method::GET;
						else if (method == "POST") header.request_method = Method::POST;
						else if (method == "OPTIONS") header.request_method = Method::OPTIONS;
						else return;
						header.path = read_word(p, (int)(line.c_str() + line.length() - p));
						if (header.path.empty()) return;//错误的请求，URL为空
						std::string p_version = read_word(p, (int)(line.c_str() + line.length() - p));
						if (p_version != protocol_version) return;//不支持的版本
						//循环读取
						bool is_true_request = false;
						while (now_ptr < end_ptr)
						{
							line = read_line(now_ptr, (int)(end_ptr - now_ptr));
							if (line == "\r\n")
							{
								is_true_request = true;
								break;
							}
							try
							{
								header.headers.AddHeader(line);
							}
							catch (const HTTPException&) {}
						}
						if (is_true_request == false) return;
						if (header.request_method == Method::POST)
						{
							//检查是否具有ContentLength
							Buffer post_content(end_ptr - now_ptr > 0 ? end_ptr - now_ptr : 1);
							if (!header.headers["Content-Length"].empty())
							{
								//存在ContentLength
								std::string str = header.headers["Content-Length"];
								int len = 0;
								std::stringstream(str) >> len;
								if (len > 0) post_content.Resize(len);
								else post_content.Resize(1);
								if (end_ptr - now_ptr > 0)
								{
									//第一次接收的还有数据
									if (end_ptr - now_ptr <= len)
									{
										post_content.Data(now_ptr, end_ptr - now_ptr);
										len -= (int)(end_ptr - now_ptr);
									}
									else
									{
										post_content.Data(now_ptr, len);
										len = 0;
									}
								}
								if (len > 0)
								{
									auto t = connection.Recv(len);
									post_content.AppendToEnd(t);
								}
							}
							else
							{
								if (end_ptr - now_ptr > 0)
								{
									post_content.Data(now_ptr, end_ptr - now_ptr);
								}
							}
							//检查是否具有Connection
							if (header.headers.ExistHeader("Connection"))
							{
								if (header.headers["Connection"] == "close")
								{
									is_keep_alive = false;
								}
							}
							if (post(connection, header, post_content))
								is_keep_alive = false;
						}
						else if (header.request_method == Method::GET)
						{
							//检查是否具有Connection
							if (header.headers.ExistHeader("Connection"))
							{
								if (header.headers["Connection"] == "close")
								{
									is_keep_alive = false;
								}
							}
							if (get(connection, header))
								is_keep_alive = false;
						}
						else if (header.request_method == Method::OPTIONS)
						{
							//检查是否具有Connection
							if (header.headers.ExistHeader("Connection"))
							{
								if (header.headers["Connection"] == "close")
								{
									is_keep_alive = false;
								}
							}
							if (options(connection, header))
								is_keep_alive = false;
						}
						if (is_keep_alive == false) break;
					}
				}
				catch (const std::exception& ex)
				{
					std::string s = ex.what();
					return;
				}
				});
		}
	}
	else
	{
		while (true)
		{
			auto connection = this->server.Accept();
			std::string& protocol_version = this->protocol_version;
			auto& get = this->on_get_request;
			auto& post = this->on_post_request;
			auto& options = this->on_options_request;
			std::thread([connection, protocol_version, get, post,options]() {
				//读取Header
				bool is_keep_alive = true;
				try
				{
					while (true)
					{
						RequestHeader header;
						const char* p;
						auto buffer = connection.Recv(1024 * 1024);
						const char* now_ptr = (const char*)buffer.Data();
						const char* end_ptr = (const char*)buffer.Data() + buffer.GetLength();
						//读取协议行
						std::string line = read_line(now_ptr, (int)(end_ptr - now_ptr));
						if (is_empty_line(line)) return;
						p = line.c_str();
						std::string method = read_word(p, (int)line.length());
						if (method == "GET") header.request_method = Method::GET;
						else if (method == "POST") header.request_method = Method::POST;
						else if (method == "OPTIONS") header.request_method = Method::OPTIONS;
						else return;
						header.path = read_word(p, (int)(line.c_str() + line.length() - p));
						if (header.path.empty()) return;//错误的请求，URL为空
						std::string p_version = read_word(p, (int)(line.c_str() + line.length() - p));
						if (p_version != protocol_version) return;//不支持的版本
						//循环读取
						bool is_true_request = false;
						while (now_ptr < end_ptr)
						{
							line = read_line(now_ptr,(int)( end_ptr - now_ptr));
							if (line == "\r\n")
							{
								is_true_request = true;
								break;
							}
							try
							{
								header.headers.AddHeader(line);
							}
							catch (const HTTPException&) {}
						}
						if (is_true_request == false) return;
						if (header.request_method == Method::POST)
						{
							//检查是否具有ContentLength
							Buffer post_content(end_ptr - now_ptr > 0 ? end_ptr - now_ptr : 1);
							if (!header.headers["Content-Length"].empty())
							{
								//存在ContentLength
								std::string str = header.headers["Content-Length"];
								int len = 0;
								std::stringstream(str) >> len;
								if (len > 0) post_content.Resize(len);
								else post_content.Resize(1);
								if (end_ptr - now_ptr > 0)
								{
									//第一次接收的还有数据
									if (end_ptr - now_ptr <= len)
									{
										post_content.Data(now_ptr, end_ptr - now_ptr);
										len -= (int)(end_ptr - now_ptr);
									}
									else
									{
										post_content.Data(now_ptr, len);
										len = 0;
									}
								}
								if (len > 0)
								{
									auto t = connection.Recv(len);
									post_content.AppendToEnd(t);
								}
							}
							else
							{
								if (end_ptr - now_ptr > 0)
								{
									post_content.Data(now_ptr, end_ptr - now_ptr);
								}
							}
							//检查是否具有Connection
							if (header.headers.ExistHeader("Connection"))
							{
								if (header.headers["Connection"] == "close")
								{
									is_keep_alive = false;
								}
							}
							if (post(connection, header, post_content))
								is_keep_alive = false;
						}
						else if (header.request_method == Method::GET)
						{
							//检查是否具有Connection
							if (header.headers.ExistHeader("Connection"))
							{
								if (header.headers["Connection"] == "close")
								{
									is_keep_alive = false;
								}
							}
							if (get(connection, header))
								is_keep_alive = false;
						}
						else if (header.request_method == Method::OPTIONS)
						{
							//检查是否具有Connection
							if (header.headers.ExistHeader("Connection"))
							{
								if (header.headers["Connection"] == "close")
								{
									is_keep_alive = false;
								}
							}
							if (options(connection, header))
								is_keep_alive = false;
						}
						if (is_keep_alive == false) break;
					}
				}
				catch (const std::exception& ex)
				{
					std::string s = ex.what();
					return;
				}
				}).detach();
		}
	}
	
}

void RbsLib::Network::HTTP::HTTPServer::SetPostHandle(const std::function<int(const TCP::TCPConnection& connection, RequestHeader& header, Buffer& post_content)>& func)
{
	this->on_post_request = func;
}

void RbsLib::Network::HTTP::HTTPServer::SetGetHandle(const std::function<int(const TCP::TCPConnection& connection, RequestHeader& header)>& func)
{
	this->on_get_request = func;
}

void RbsLib::Network::HTTP::HTTPServer::SetOptionsHandle(const std::function<int(const TCP::TCPConnection& connection, RequestHeader& header)>& func)
{
	this->on_options_request = func;
}

void RbsLib::Network::HTTP::HTTPServer::StopAndThrowExceptionInLoopThread(void)
{
	this->server.Disable();
}


RbsLib::Network::HTTP::HTTPException::HTTPException(const std::string& reason) noexcept
	:reason(reason)
{
}

const char* RbsLib::Network::HTTP::HTTPException::what(void) const noexcept
{
	return this->reason.c_str();
}

void RbsLib::Network::HTTP::HTTPHeadersContent::AddHeader(const std::string& key, const std::string& value)
{
	if (key.empty() || value.empty()) throw HTTPException("Add non key-value to headers content");
	for (char it: key) if (!std::isprint(it)) throw HTTPException("Add invaild key to headers content");
	for (char it : value) if (!std::isprint(it)) throw HTTPException("Add invaild value to headers content");
	this->headers[key] = value;
}

void RbsLib::Network::HTTP::HTTPHeadersContent::AddHeader(const std::string& line)
{
	std::cmatch m;
	static const std::regex reg("^\\s*([a-z,A-Z,_,-]+)\\s*:\\s*([^\\f\\n\\r\\t\\v]+)\\s*$");
	std::regex_match(line.c_str(), m, reg);
	if (m.size() != 3) throw HTTPException("HTTP Header line parse failed");
	this->headers[m[1]] = m[2];
}

auto RbsLib::Network::HTTP::HTTPHeadersContent::GetHeader(const std::string& key) const -> std::string
{
	for (const auto& it: this->headers)
	{
		auto iter_a = it.first.begin(), iter_b = key.begin();
		while (iter_a != it.first.end() && iter_b != key.end())
		{
			if (std::tolower(*iter_a) != std::tolower(*iter_b)) break;
			++iter_a;
			++iter_b;
		}
		if (iter_a == it.first.end() && iter_b == key.end())
		{
			return it.second;
		}
	}
	return std::string();
}

auto RbsLib::Network::HTTP::HTTPHeadersContent::operator[](const std::string& key)const -> std::string
{
	return this->GetHeader(key);
}

auto RbsLib::Network::HTTP::HTTPHeadersContent::Headers(void)const -> const std::map<std::string, std::string>&
{
	return this->headers;
}

bool RbsLib::Network::HTTP::HTTPHeadersContent::ExistHeader(const std::string& key) const noexcept
{
	for (const auto& it : this->headers)
	{
		auto iter_a = it.first.begin(), iter_b = key.begin();
		while (iter_a != it.first.end() && iter_b != key.end())
		{
			if (std::tolower(*iter_a) != std::tolower(*iter_b)) break;
			++iter_a;
			++iter_b;
		}
		if (iter_a == it.first.end() && iter_b == key.end())
		{
			return true;
		}
	}
	return false;
}

auto RbsLib::Network::HTTP::HTTPHeadersContent::GetHeaderMap(void) const noexcept -> const std::map<std::string, std::string>&
{
	return this->headers;
}

std::string RbsLib::Network::HTTP::RequestHeader::ToString(void) const noexcept
{
	std::string str;
	switch (this->request_method)
	{
	default:
	case Method::GET:
		str = "GET ";
		break;
	case Method::POST:
		str = "POST ";
		break;
	}
	if (this->path.empty()||this->path[0]!='/') str += "/";
	str += this->path+' ';
	str += this->version+"\r\n";
	for (const auto& it : this->headers.Headers())
	{
		str += it.first+": ";
		str += it.second + "\r\n";
	}
	return str + "\r\n";
}

auto RbsLib::Network::HTTP::RequestHeader::ToBuffer(void) const noexcept -> Buffer
{
	return RbsLib::Buffer(this->ToString());
}

std::string RbsLib::Network::HTTP::ResponseHeader::ToString(void) const noexcept
{
	std::string res = this->version + ' ' + std::to_string(this->status) + ' ' + this->status_descraption + "\r\n";
	for (const auto& it : this->headers.Headers())
	{
		res += it.first+": "+it.second+"\r\n";
	}
	res += "\r\n";
	return res;
}

auto RbsLib::Network::HTTP::ResponseHeader::ToBuffer(void) const noexcept -> RbsLib::Buffer
{
	return RbsLib::Buffer(this->ToString());
}

RbsLib::Network::HTTP::Request::Request(const TCP::TCPConnection& connection, const RequestHeader& header, const Buffer& buffer)
	:connection(connection),header(header),content(buffer)
{
}

RbsLib::Network::UDP::UDPServer::UDPServer()
{
	net::init_network();
	this->sock = std::shared_ptr<SOCKET>(new SOCKET(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)), [](SOCKET* sock) {
		if (sock && *sock != INVALID_SOCKET)
		{
#ifdef WIN32
			closesocket(*sock);
#endif
#ifdef LINUX
			close(*sock);
#endif
		}
		delete sock;
		});
	this->is_bind=std::make_shared<bool>(false);
	if (*this->sock == INVALID_SOCKET) throw net::NetworkException("Allocate socket failed");
}

RbsLib::Network::UDP::UDPServer::UDPServer(int port, const std::string& address)
	:UDPServer()
{
	this->Bind(port, address);
}

void RbsLib::Network::UDP::UDPServer::Bind(int port, const std::string& address)
{
	struct sockaddr_in s_sin;
	if (*this->is_bind) throw net::NetworkException("This object is already bind");
	if (port < 0 || port>65535) throw net::NetworkException("Port mast be in range 0-65535");
	s_sin.sin_family = AF_INET;
	s_sin.sin_port = htons(port);
	s_sin.sin_addr.s_addr = inet_addr(address.c_str());
	if (bind(*this->sock, (struct sockaddr*)&s_sin, sizeof(s_sin)) != 0)
	{
		std::string reason = "Bind failed";
		#ifdef LINUX
		reason += ": ";
		reason += strerror(errno);
		#endif
	}
	*this->is_bind = true;
}

void RbsLib::Network::UDP::UDPServer::Close(void)
{
	this->is_bind = nullptr;
	this->sock = nullptr;
}

void RbsLib::Network::UDP::UDPServer::Send(const std::string& ip, int port, const RbsLib::IBuffer& buffer) const
{
	//构建目标地址
	struct sockaddr_in s_sin = {0};
	s_sin.sin_family = AF_INET;
	s_sin.sin_port = htons(port);
	s_sin.sin_addr.s_addr = inet_addr(ip.c_str());
	if (sendto(*this->sock, (const char*)buffer.Data(), buffer.GetLength(), 0, (struct sockaddr*)&s_sin, sizeof(s_sin)) != buffer.GetLength())
	{
		throw net::NetworkException("Send failed");
	}
}

void RbsLib::Network::UDP::UDPServer::Send(const std::string& ip, int port, const void* data, int len) const
{
	//构建目标地址
	struct sockaddr_in s_sin = { 0 };
	s_sin.sin_family = AF_INET;
	s_sin.sin_port = htons(port);
	s_sin.sin_addr.s_addr = inet_addr(ip.c_str());
	if (sendto(*this->sock, (const char*)data, len, 0, (struct sockaddr*)&s_sin, sizeof(s_sin)) != len)
	{
		throw net::NetworkException("Send failed");
	}
}

void RbsLib::Network::UDP::UDPServer::Send(const UDPDatagram& datagram) const
{
	if (sendto(*this->sock, (const char*)datagram.GetBuffer().Data(), datagram.GetBuffer().GetLength(), 0, (struct sockaddr*)&datagram.GetConnectionInfo(), sizeof(struct sockaddr_in)) != datagram.GetBuffer().GetLength())
	{
		throw net::NetworkException("Send failed");
	}
}

auto RbsLib::Network::UDP::UDPServer::Recv(int max_len) const ->UDPDatagram
{
	struct sockaddr_in info;
	socklen_t info_len = sizeof(info);
	std::unique_ptr<char[]> data=std::make_unique<char[]>(max_len);
	int s;
	if ((s = recvfrom(*this->sock, data.get(), max_len, 0, (struct sockaddr*)&info, &info_len)) <= 0)
	{
		throw net::NetworkException("Recv failed");
	}
	RbsLib::Buffer buffer(data.get(), s);
	return UDPDatagram(info, buffer);
}


RbsLib::Network::UDP::UDPDatagram::UDPDatagram(const sockaddr_in& connection_info, const RbsLib::Buffer& buffer)
	:connection_info(connection_info),buffer(buffer)
{
}

sockaddr_in& RbsLib::Network::UDP::UDPDatagram::GetConnectionInfo(void) const noexcept
{
	return const_cast<sockaddr_in&>(this->connection_info);
}

RbsLib::Buffer& RbsLib::Network::UDP::UDPDatagram::GetBuffer(void) const noexcept
{
	return const_cast<RbsLib::Buffer&>(this->buffer);
}

std::string RbsLib::Network::UDP::UDPDatagram::GetAddress(void) const noexcept
{
	return inet_ntoa(this->connection_info.sin_addr);
}

int RbsLib::Network::UDP::UDPDatagram::GetPort(void) const noexcept
{
	return ntohs(this->connection_info.sin_port);
}

RbsLib::Network::UDP::UDPClient::UDPClient()
{
	net::init_network();
	this->sock = std::shared_ptr<SOCKET>(new SOCKET(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)), [](SOCKET* sock) {
		if (sock && *sock != INVALID_SOCKET)
		{
#ifdef WIN32
			closesocket(*sock);
#endif
#ifdef LINUX
			close(*sock);
#endif
		}
		delete sock;
		});
	if (*this->sock == INVALID_SOCKET) throw net::NetworkException("Allocate socket failed");
}

void RbsLib::Network::UDP::UDPClient::Send(const std::string& ip, int port, const RbsLib::IBuffer& buffer) const
{
	//构建目标地址
	struct sockaddr_in s_sin = { 0 };
	s_sin.sin_family = AF_INET;
	s_sin.sin_port = htons(port);
	s_sin.sin_addr.s_addr = inet_addr(ip.c_str());
	if (sendto(*this->sock, (const char*)buffer.Data(), buffer.GetLength(), 0, (struct sockaddr*)&s_sin, sizeof(s_sin)) != buffer.GetLength())
	{
		throw net::NetworkException("Send failed");
	}
}

void RbsLib::Network::UDP::UDPClient::Send(const std::string& ip, int port, const void* data, int len) const
{
	//构建目标地址
	struct sockaddr_in s_sin = { 0 };
	s_sin.sin_family = AF_INET;
	s_sin.sin_port = htons(port);
	s_sin.sin_addr.s_addr = inet_addr(ip.c_str());
	if (sendto(*this->sock, (const char*)data, len, 0, (struct sockaddr*)&s_sin, sizeof(s_sin)) != len)
	{
		throw net::NetworkException("Send failed");
	}
}

void RbsLib::Network::UDP::UDPClient::Send(const UDPDatagram& datagram) const
{
	if (sendto(*this->sock, (const char*)datagram.GetBuffer().Data(), datagram.GetBuffer().GetLength(), 0, (struct sockaddr*)&datagram.GetConnectionInfo(), sizeof(struct sockaddr_in)) != datagram.GetBuffer().GetLength())
	{
		throw net::NetworkException("Send failed");
	}
}

auto RbsLib::Network::UDP::UDPClient::Recv(const std::string& ip, int port, int max_len) const->UDPDatagram
{
	struct sockaddr_in info;
	socklen_t info_len = sizeof(info);
	std::unique_ptr<char[]> data = std::make_unique<char[]>(max_len);
	int s;
	//构建目标地址
	struct sockaddr_in s_sin = { 0 };
	s_sin.sin_family = AF_INET;
	s_sin.sin_port = htons(port);
	s_sin.sin_addr.s_addr = inet_addr(ip.c_str());
	if ((s = recvfrom(*this->sock, data.get(), max_len, 0, (struct sockaddr*)&info, &info_len)) <= 0)
	{
		throw net::NetworkException("Recv failed");
	}
	RbsLib::Buffer buffer(data.get(), s);
	return UDPDatagram(info, buffer);
}

auto RbsLib::Network::UDP::UDPClient::Recv(const UDPDatagram& datagram) const->UDPDatagram
{
	struct sockaddr_in info;
	socklen_t info_len = sizeof(info);
	std::unique_ptr<char[]> data = std::make_unique<char[]>(datagram.GetBuffer().GetLength());
	int s;
	if ((s = recvfrom(*this->sock, data.get(), datagram.GetBuffer().GetLength(), 0, (struct sockaddr*)&info, &info_len)) <= 0)
	{
		throw net::NetworkException("Recv failed");
	}
	RbsLib::Buffer buffer(data.get(), s);
	return UDPDatagram(info, buffer);
}

RbsLib::Network::TCP::TCPStream::TCPStream(const TCPConnection& connection)
	:connection(connection)
{
}

const RbsLib::Buffer& RbsLib::Network::TCP::TCPStream::Read(Buffer& buffer, int64_t size)
{
    buffer.Resize(size);
    int64_t bytesRead = connection.Recv(buffer, 0);
    buffer.Resize(bytesRead);
    return buffer;
}

int64_t RbsLib::Network::TCP::TCPStream::Read(void* ptr, int64_t size)
{
    return connection.Recv(ptr, size, 0);
}

void RbsLib::Network::TCP::TCPStream::Write(const IBuffer& buffer)
{
    connection.Send(buffer, 0);
}

void RbsLib::Network::TCP::TCPStream::Write(const void* ptr, int64_t size)
{
    connection.Send(ptr, size, 0);
}

auto RbsLib::Network::Address::GetAddresses(const std::string& address, int port) -> std::list<Address>
{
	std::list<Address> addresses;
	struct addrinfo* answer = nullptr, hint = { 0 };
	hint.ai_family = AF_UNSPEC;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_protocol = IPPROTO_TCP;
	if (getaddrinfo(address.c_str(), std::to_string(port).c_str(), &hint, &answer)) {
		throw net::NetworkException("Get address info failed");
	}
	for (struct addrinfo* it = answer; it; it = it->ai_next)
	{
		Address addr;
		addr.family = it->ai_family;
		addr.SetAddressStructure(it->ai_addr, static_cast<socklen_t>(it->ai_addrlen));
		addresses.emplace_back(addr);
	}
	freeaddrinfo(answer);
	return addresses;
}

const struct sockaddr* RbsLib::Network::Address::GetAddressStructure(void) const
{
	return (const sockaddr*)this->address.Data();
}

void RbsLib::Network::Address::SetAddressStructure(const sockaddr* in, socklen_t len)
{
	this->address = RbsLib::Buffer(in, static_cast<std::uint64_t>(len));
}

auto RbsLib::Network::Address::GetAddressStructureLength(void) const -> socklen_t
{
	return static_cast<socklen_t>(this->address.GetLength());
}
