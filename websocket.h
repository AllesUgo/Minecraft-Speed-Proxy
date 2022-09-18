#pragma once
#include<stdio.h>
#include<stdlib.h>

#define IP_LEN 16

#ifdef _WIN32
#include<WinSock2.h>
#include<winsock.h>
#pragma comment(lib,"ws2_32.lib")
typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;
#endif // _WIN32

#ifndef _WIN32
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
typedef int SOCKET;
#endif

typedef SOCKET WS_ServerPort_t;
typedef struct WS_CONNECTINFO {
	SOCKET sock;
	char addr[IP_LEN];
} WS_Connection_t;

//server
WS_ServerPort_t WS_CreateServerPort(int port,int maxlixt);//创建服务端口
int WS_WaitClient(WS_ServerPort_t, WS_Connection_t* connection);//等待客户端连接


//client
int WS_ConnectServer(const char*, int port, WS_Connection_t* connection);

//ALL
int WS_Send(WS_Connection_t*, const void*, size_t);//向目标发送数据
int WS_Recv(WS_Connection_t*,void*, size_t);//接收目标发来的数据
void WS_CloseConnection(WS_Connection_t*);//断开连接