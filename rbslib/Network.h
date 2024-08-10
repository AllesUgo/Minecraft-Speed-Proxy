#pragma once
#include <exception>
#include <string>
#include <mutex>
#include <functional>
#include <map>
#include <list>
#include <memory>
#include "BaseType.h"
#include "Buffer.h"
#include "Streams.h"
#ifdef WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
typedef int socklen_t;
#endif // win32

#ifdef LINUX
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR SO_ERROR
typedef int SOCKET;
#endif
namespace RbsLib
{
	namespace Network
	{
#ifdef WIN32
		static bool network_inited = 0;
#endif // WIN32
		void init_network();
		class Address 
		{
		public:
			int family;
			static auto GetAddresses(const std::string& address, int port)->std::list<Address>;
			const struct sockaddr* GetAddressStructure(void)const;
			void SetAddressStructure(const struct sockaddr* in, socklen_t len);
			auto GetAddressStructureLength(void)const->socklen_t;
		private:
			RbsLib::Buffer address;
		};

		class NetworkException :public std::exception
		{
		private:
			std::string reason;
		public:
			NetworkException(const std::string& str)noexcept :reason(str) {}
			const char* what(void)const noexcept override { return reason.c_str(); }
		};
		namespace TCP
		{
			class TCPServer;
			class TCPConnection;
			class TCPClient;
			class TCPStream;
			class TCPConnection
			{
				/*同一时间同一连接对象只能有一个线程进行拷贝、移动、析构*/
			private:
				SOCKET sock;
				union{
					struct sockaddr_in connection_info;
					struct sockaddr_in6 connection_info6;
				} connection_info;
				bool is_ipv6;
				//int info_len;
				std::mutex* mutex = nullptr;
				int* reference_counter = nullptr;
				TCPConnection(SOCKET sock, const struct sockaddr_in& connection_info, int info_len)noexcept;
				TCPConnection(SOCKET sock, const struct sockaddr_in6& connection_info, int info_len)noexcept;
				friend class TCPServer;
				friend class TCPClient;
			public:
				TCPConnection(const TCPConnection& connection) noexcept;
				TCPConnection(TCPConnection&& connection) noexcept;
				~TCPConnection(void)noexcept;
				const RbsLib::Network::TCP::TCPConnection& operator=(const TCPConnection& conection) noexcept;
				const RbsLib::Network::TCP::TCPConnection& operator=(TCPConnection&& connection) noexcept;
				bool operator==(const TCPConnection& connection)const noexcept;
				void Send(const RbsLib::IBuffer& buffer, int flag = 0) const;
				int Send(const void* data, int len, int flag = 0) const noexcept;
				RbsLib::Buffer Recv(int len, int flag = 0) const;
				int Recv(RbsLib::Buffer& buffer, int flag = 0) const;
				int Recv(void* data, int len, int flag = 0)const noexcept;
				SOCKET GetSocket(void)const noexcept;
				std::string GetAddress(void)const noexcept;
				void Close(void);
				void SetSocketOption(int level, int optname, const void* optval, socklen_t optlen) const;
				void Disable(void) const;
			};
			class TCPServer
			{
			private:
				SOCKET server_socket = 0;
				bool is_listen = false;
				bool is_bind = false;
				int* reference_counter = nullptr;
				bool is_ipv6 = false;
				bool is_force_closed = false;
			public:
				TCPServer();
				TCPServer(int port, const std::string& address = "0.0.0.0",bool is_ipv6=false);
				TCPServer(const TCPServer& server) noexcept;
				~TCPServer(void)noexcept;
				const TCPServer& operator=(const TCPServer& server)noexcept;
				SOCKET GetSocket(void)const noexcept;
				void Bind(int port, const std::string& address = "0.0.0.0");
				void Listen(int listen_num = 5);
				RbsLib::Network::TCP::TCPConnection Accept(void);
				void Close(void) noexcept;
				void ForceClose(void);
			};
			class TCPClient
			{
			public:
				static RbsLib::Network::TCP::TCPConnection Connect(std::string ip, int port);
				static auto Connect6(std::string ip, int port)->RbsLib::Network::TCP::TCPConnection;
				static auto Connect(const Address& addr) -> RbsLib::Network::TCP::TCPConnection;
			};
            /**
             * @class TCPStream
             * @brief Represents a TCP stream for reading from and writing to a TCP connection.
             * @details This class inherits from RbsLib::Streams::IOStream and provides methods for reading and writing data.
             */
            class TCPStream : public RbsLib::Streams::IOStream
            {
            private:
                TCPConnection connection; /**< The TCP connection associated with the stream. */

            public:
                /**
                 * @brief Constructs a TCPStream object with the specified TCP connection.
                 * @param connection The TCP connection to associate with the stream.
                 */
                TCPStream(const TCPConnection& connection);

                // Inherited from RbsLib::Streams::IOStream

                /**
                 * @brief Reads data from the stream into the specified buffer.
                 * @param buffer The buffer to read the data into.
                 * @param size The number of bytes to read.
                 * @return A reference to the buffer containing the read data.
                 */
                const Buffer& Read(Buffer& buffer, int64_t size) override;

                /**
                 * @brief Reads data from the stream into the specified memory location.
                 * @param ptr The memory location to read the data into.
                 * @param size The number of bytes to read.
                 * @return The number of bytes read.
                 */
                int64_t Read(void* ptr, int64_t size) override;

                /**
                 * @brief Writes the specified buffer to the stream.
                 * @param buffer The buffer to write to the stream.
                 */
                void Write(const IBuffer& buffer) override;

                /**
                 * @brief Writes the specified data to the stream.
                 * @param ptr The memory location of the data to write.
                 * @param size The number of bytes to write.
                 */
                void Write(const void* ptr, int64_t size) override;
            };
		}
		namespace HTTP
		{
			
			class HTTPServer;
			class HTTPHeadersContent;
			class RequestHeader;
			class Request;
			class ResponseHeader;
			class HTTPException;
			enum class Method
			{
				GET,POST
			};
			class HTTPException : public std::exception
			{
				std::string reason;
			public:
				HTTPException(const std::string& reason) noexcept;
				const char* what(void) const noexcept override;
			};
			class HTTPHeadersContent
			{
			private:
				std::map<std::string, std::string> headers;
			public:
				void AddHeader(const std::string& key, const std::string& value);
				void AddHeader(const std::string& line);
				auto GetHeader(const std::string& key) const->std::string;
				auto operator[](const std::string& key) const ->std::string ;
				auto Headers(void)const -> const std::map<std::string, std::string>& ;
			};
			class RequestHeader
			{
			public:
				Method request_method;
				const std::string version = "HTTP/1.1";
				std::string path="/";
				HTTPHeadersContent headers;
				std::string ToString(void) const noexcept;
				auto ToBuffer(void) const noexcept->RbsLib::Buffer;
			};
			class ResponseHeader
			{
			public:
				const std::string version = "HTTP/1.1";
				int status = 200;
				std::string status_descraption="ok";
				HTTPHeadersContent headers;
				std::string ToString(void) const noexcept;
				auto ToBuffer(void) const noexcept->RbsLib::Buffer;
			};
			class Request
			{
			public:
				TCP::TCPConnection connection;
				RequestHeader header;
				Buffer content;
				Request(const TCP::TCPConnection& connection,const RequestHeader&header,const Buffer&buffer);
			};
			
			class HTTPServer
			{
			private:
				TCP::TCPServer server;
				std::string protocol_version = "HTTP/1.1";
				std::function<void(const TCP::TCPConnection& connection,RequestHeader&header)> on_get_request;
				std::function<void(const TCP::TCPConnection& connection, RequestHeader& header,Buffer&post_content)> on_post_request;
			public:
				HTTPServer(const std::string& host = "0.0.0.0", int port = 80);
				HTTPServer(int port);
				void LoopWait(bool use_thread_pool = false, int keep_threads_number = 0);
				void SetPostHandle(const std::function<void(const TCP::TCPConnection& connection, RequestHeader& header, Buffer& post_content)>& func);
				void SetGetHandle(const std::function<void(const TCP::TCPConnection& connection, RequestHeader& header)>& func);
			};
		}
		namespace UDP
		{
			class UDPDatagram
			{
			private:
				struct sockaddr_in connection_info;
				RbsLib::Buffer buffer;
			public:
				UDPDatagram(const struct sockaddr_in& connection_info, const RbsLib::Buffer& buffer);
				struct sockaddr_in& GetConnectionInfo(void)const noexcept;
				RbsLib::Buffer& GetBuffer(void)const noexcept;
				std::string GetAddress(void)const noexcept;
				int GetPort(void)const noexcept;
			};
			class UDPServer
			{
			private:
				std::shared_ptr<SOCKET> sock=nullptr;
				std::shared_ptr<bool> is_bind=nullptr;
			public:
				UDPServer();
				UDPServer(int port, const std::string& address = "");
				void Bind(int port, const std::string& address = "");
				void Close(void);
				void Send(const std::string& ip, int port, const RbsLib::IBuffer& buffer)const ;
				void Send(const std::string& ip, int port, const void* data, int len)const;
				void Send(const UDPDatagram& datagram)const;
				UDPDatagram Recv(int max_len)const;
			};
			class UDPClient
			{
			private:
				std::shared_ptr<SOCKET> sock = nullptr;
			public:
				UDPClient();
				void Send(const std::string& ip, int port, const RbsLib::IBuffer& buffer)const;
				void Send(const std::string& ip, int port, const void* data, int len)const;
				void Send(const UDPDatagram& datagram)const;
				UDPDatagram Recv(const std::string& ip,int port,int max_len)const;
				UDPDatagram Recv(const UDPDatagram& datagram)const;
			};
		}
	}
}
