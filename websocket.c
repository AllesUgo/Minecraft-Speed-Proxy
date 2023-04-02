#include"websocket.h"

WS_ServerPort_t WS_CreateServerPort(int port,int maxlist);
int WS_WaitClient(WS_ServerPort_t, WS_Connection_t* connection);
void WS_CloseConnection(WS_Connection_t*);
int WS_Send(WS_Connection_t*, const void*, size_t);
int WS_Recv(WS_Connection_t*, void*, size_t);
int WS_ConnectServer(const char*, int port, WS_Connection_t* connection);

int WS_WaitClient(WS_ServerPort_t serverport, WS_Connection_t* connection)
{
	//Windows Linux通用
	WS_Connection_t client;
	struct sockaddr_in remoteAddr;
	socklen_t nAddrlen = (unsigned int)sizeof(remoteAddr);
	client.sock = accept(serverport, (struct sockaddr*)&remoteAddr, &nAddrlen);//连接客户端
	if (client.sock<=0) return -1;
	strcpy(client.addr, inet_ntoa(remoteAddr.sin_addr));
	memcpy(connection, &client, sizeof(client));
	return 0;
}
#ifdef _WIN32
int WS_Send(WS_Connection_t* client, const void* buff, size_t datasize)
{
	return send(client->sock, buff, datasize, 0);

}int WS_Recv(WS_Connection_t* client, void* buff, size_t max_recv_size)
{
	return recv(client->sock, buff, max_recv_size, 0);
}

WS_ServerPort_t WS_CreateServerPort(int port, int maxlist)
{
	WSADATA wsaData;
	int r = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (r != 0)
		return 0;
	SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_socket == 0)
	{
		//申请套接字失败
		return 0;
	}
	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.S_un.S_addr = INADDR_ANY;
	if (bind(server_socket, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR)
	{
		//绑定失败，可能是端口被占用
		return 0;
	}
	if (listen(server_socket, maxlist) == SOCKET_ERROR)
	{
		return 0;
	}
	return server_socket;

}

void WS_CloseConnection(WS_Connection_t* client)
{
	closesocket(client->sock);
}

int WS_ConnectServer(const char* address, int port, WS_Connection_t* connection)
{
	WS_Connection_t server;
	server.sock = 0;
	int IP;
	//初始化连接
	WSADATA wsaData;
	int r = WSAStartup(MAKEWORD(2, 2), &wsaData);
	//解析域名
	struct hostent* host = gethostbyname(address);
	if (!host)
	{
		return -1;//找不到服务器
	}
	
	//进行连接
	SOCKET sclient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sclient == INVALID_SOCKET)
	{
		return -2;//套接字创建失败
	}

	sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(port);
	serAddr.sin_addr.S_un.S_addr = inet_addr(inet_ntoa(*(struct in_addr*)host->h_addr_list[0]));
	if (connect(sclient, (sockaddr*)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
	{
		return -10;//出现错误
	}
	strncpy(server.addr, inet_ntoa(*(struct in_addr*)host->h_addr_list[0]), 16);
	server.sock = sclient;
	memcpy(connection, &sclient, sizeof(sclient));
	return 0;
}
#endif // wnidows

#ifndef _WIN32
int WS_Send(WS_Connection_t* client, const void* buff, size_t datasize)
{
	return send(client->sock, buff, datasize, MSG_NOSIGNAL);

}int WS_Recv(WS_Connection_t* client, void* buff, size_t max_recv_size)
{
	return recv(client->sock, buff, max_recv_size, MSG_NOSIGNAL);
}

WS_ServerPort_t WS_CreateServerPort(int port, int maxlist)
{
	
	SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_socket == -1)
	{
		//申请套接字失败
		return 0;
	}
	int opt = 1;
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); 
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = INADDR_ANY;
	if (bind(server_socket,(struct sockaddr *)(&sin), sizeof(sin)) == SOCKET_ERROR)
	{
		//绑定失败，可能是端口被占用
		return 0;
	}
	if (listen(server_socket, maxlist) == SOCKET_ERROR)
	{
		return 0;
	}
	return server_socket;

}

void WS_CloseConnection(WS_Connection_t* client)
{
	close(client->sock);
}

int WS_ConnectServer(const char* address, int port, WS_Connection_t* connection)
{
	WS_Connection_t server;
	server.sock = 0;
	//解析域名
	struct hostent* host = gethostbyname(address);
	if (!host)
	{
		return -1;//找不到服务器
	}
	
	//进行连接
	SOCKET sclient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sclient == INVALID_SOCKET)
	{
		return -2;//套接字创建失败
	}

	struct sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(port);
	serAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)host->h_addr_list[0]));
	if (connect(sclient, (struct sockaddr*)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
	{
		return -10;//出现错误
	}
	strncpy(server.addr, inet_ntoa(*(struct in_addr*)host->h_addr_list[0]), 16);
	server.sock = sclient;
	memcpy(connection, &sclient, sizeof(sclient));
	return 0;
}
#endif//linux
