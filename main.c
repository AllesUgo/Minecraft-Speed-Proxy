#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include "websocket.h"
#include "cjson/cJSON.h"
#include "log.h"
 char *Version;
char *remoteServerAddress;
int LocalPort;
int Remote_Port;
char *jdata;
void OnlineControl_Init();
void *DealClient(void *InputArg);
void addip(int sock, const char *IP);
void sighandle(int sig);
void sighandle(int sig)
{
	switch (sig)
	{
	case SIGINT:
		printf("[%s] 退出服务器\n", gettime().time);
		exit(0);
		break;
	case SIGSEGV:
		printf("[%s] [E] 服务器内部错误，请重新启动服务进程，您也可以将error.log发送给开发人员\n", gettime().time);
		system("date >>error.log");
		system("uname -a >>error.log");
		system("echo ulimit: >>error.log");
		system("ulimit -s>>error.log");
		exit(1);
	default:
		break;
	}
}
int main(int argc, char *argv[])
{
	signal(SIGINT, sighandle);
	signal(SIGSEGV, sighandle);
	signal(SIGPIPE, SIG_IGN);
	Version = (char *)malloc(1024);
	sprintf(Version, "Version:2.0.0\n编译时间:%s\n编译器版本:%s\n", __DATE__, __VERSION__);
	char noinput_sign = 0;
	if (argc == 2 && (!strcmp(argv[1], "--version")))
	{

		puts(Version);
		return 0;
	}
	else
	{
		if (argc < 4)
		{
			printf("参数错误！\n<远程服务器地址> <远程服务器端口> <本地监听端口> [可选参数]\t启动服务器\n--version\t显示版本信息\n可选参数:\n\t --noinput 无命令控制\n");
			return 1;
		}
		if (argc > 4)
		{
			char unknown_sign = 0;
			for (int i = 4; i < argc; i++)
			{
				if (!strcmp(argv[i], "--noinput"))
				{
					noinput_sign = 1;
					printf("禁用命令行控制\n");
				}
				else
				{
					printf("未知的参数 %s\n", argv[i]);
					unknown_sign = 1;
				}
			}
			if (unknown_sign == 1)
			{
				return 1;
			}
		}
	}

	remoteServerAddress = (char *)malloc(strlen(argv[1] + 1));
	strcpy(remoteServerAddress, argv[1]);
	sscanf(argv[2], "%d", &Remote_Port);
	sscanf(argv[3], "%d", &LocalPort);
	printf("[%s] [I] PID:%d 远程服务器:%s:%d 本地监听端口%d\n", gettime().time, getpid(), remoteServerAddress, Remote_Port, LocalPort);
	//读取motd
	printf("[%s] [I] 加载motd数据\n", gettime().time);
	FILE *fp = fopen("motd.json", "r");
	if (fp == NULL)
	{
		printf("[%s] [W] 没有找到motd.json\n", gettime().time);
		jdata = (char *)malloc(141);
		strcpy(jdata, "{\"version\": {\"name\": \"1.8.7\",\"protocol\": 47},\"players\": {\"max\": 0,\"online\": 0,\"sample\": []},\"description\": {\"text\": \"Minecraft Speed Plus\"}}");
		fp = fopen("motd.json", "w");
		if (fp == NULL)
		{
			printf("[%s] [W] 无法保存motd.json文件，原因是:%s\n", gettime().time, strerror(errno));
		}
		else
		{
			fputs(jdata, fp);
			fclose(fp);
			printf("[%s] [I] 已生成默认的motd.json\n", gettime().time);
		}
	}
	else
	{
		int filesize;
		fseek(fp, 0L, SEEK_END);
		filesize = ftell(fp);
		fseek(fp, 0L, SEEK_SET);
		jdata = (char *)malloc(filesize);
		fread(jdata, filesize, 1, fp);
		fclose(fp);
	}
	if (noinput_sign == 0)
	{
		printf("[%s] [I] 初始化在线人数管理\n", gettime().time);
		OnlineControl_Init();
	}
	//创建监听端口
	printf("[%s] [I] 初始化服务端口\n", gettime().time);
	WS_ServerPort_t server = WS_CreateServerPort(LocalPort, 5);
	if (server == 0)
	{
		if (errno == 98)
			printf("[%s] [E] 绑定端口%d失败，端口已被占用\n", gettime().time, LocalPort);
		return 1;
	}
	//循环等待用户连接
	WS_Connection_t *client;
	pthread_t pid;
	printf("[%s] [I] 加载完成，等待连接，输入help获取帮助\n", gettime().time);
	while (1)
	{
		client = (WS_Connection_t *)malloc(sizeof(WS_Connection_t));
		if (-1 == WS_WaitClient(server, client))
		{
			//建立连接失败
			printf("[%s] [W] 连接建立失败:%s\n", gettime().time, strerror(errno));
			free(client);
			continue;
		}
		addip(client->sock, client->addr);
		if (0 != pthread_create(&pid, NULL, DealClient, client))
		{
			printf("[%s] [E] 创建线程失败\n", gettime().time);
			WS_CloseConnection(client);
			free(client);
			sleep(5);
			continue;
		}
		pthread_detach(pid);
	}
}
