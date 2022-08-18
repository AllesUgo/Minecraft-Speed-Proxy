#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include "websocket.h"
#include "cJSON.h"
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
		if (argc < 3)
		{
			printf("参数错误！\n<远程服务器地址> <远程服务器端口> <本地监听端口> [可选参数]\t启动服务器\n--version\t显示版本信息\n可选参数:\n\t --noinput 无命令控制\n");
			return 1;
		}
		else if (argc == 3)
		{
			if (!strcmp(argv[1], "-c"))
			{
				FILE *fp = fopen(argv[2], "r");
				if (fp == NULL)
				{
					printf("无法打开文件%s,原因:%s\n", argv[2], strerror(errno));
					return 0;
				}
				//获取文件大小
				fseek(fp, 0L, SEEK_END);
				size_t filesize = ftell(fp);
				fseek(fp, 0L, SEEK_SET);
				if (filesize == 0 || filesize > 1024 * 1024)
				{
					printf("配置文件大小错误:%lu\n", filesize);
					fclose(fp);
					return 1;
				}
				char *jsondata = (char *)malloc(filesize);
				if (1 != fread(jsondata, filesize, 1, fp))
				{
					printf("读取配置文件错误\n");
					fclose(fp);
					free(jsondata);
					return 1;
				}
				fclose(fp);
				cJSON *json = cJSON_Parse(jsondata);
				free(jsondata);
				cJSON *temp = cJSON_GetObjectItem(json, "Address");
				if (temp == NULL || temp->type != cJSON_String)
				{
					printf("配置文件错误，不存在Address或其键值没有使用字符串\n");
					cJSON_Delete(json);
					return 1;
				}
				remoteServerAddress = (char *)malloc(strlen(temp->valuestring) + 1);
				strcpy(remoteServerAddress, temp->valuestring);
				temp = cJSON_GetObjectItem(json, "RemotePort");
				if (temp == NULL || temp->type != cJSON_Number)
				{
					printf("配置文件错误，不存在RemotePort或其键值没有使用整数,默认使用25565端口\n");
					Remote_Port = 25565;
				}else
				{
					Remote_Port=temp->valueint;
				}
				temp = cJSON_GetObjectItem(json, "LocalPort");
				if (temp == NULL || temp->type != cJSON_Number)
				{
					printf("配置文件错误，不存在LocalPort或其键值没有使用整数,默认使用25565端口\n");
					LocalPort = 25565;
				}else
				{
					LocalPort=temp->valueint;
				}
				temp = cJSON_GetObjectItem(json, "AllowInput");
				if (temp == NULL || (temp->type !=cJSON_True&&temp->type!=cJSON_False) )
				{
					printf("配置文件错误，不存在AllowInput或其键值没有使用布尔值,默认开启命令行控制\n");
					noinput_sign = 0;
				}
				else
				{
					if (temp->valueint == 0)
					{
						noinput_sign = 1;
						printf("禁用命令行控制\n");
					}
				}
				cJSON_Delete(json);
			}
			else
			{
				printf("不支持的参数%s\n", argv[1]);
				return 1;
			}
		}
		else if (argc >= 4)
		{
			remoteServerAddress = (char *)malloc(strlen(argv[1] + 1));
			strcpy(remoteServerAddress, argv[1]);
			sscanf(argv[2], "%d", &Remote_Port);
			sscanf(argv[3], "%d", &LocalPort);
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
