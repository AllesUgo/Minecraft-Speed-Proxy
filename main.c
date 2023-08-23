#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>
#include "websocket.h"
#include "cJSON.h"
#include "log.h"
#include "CheckLogin.h"
#include "dump.h"

const char *DVERSION = VERSION;
char *Version;
char *remoteServerAddress;
const char *Defalut_Configfile_Path = "/etc/minecraftspeedproxy/config.json";
const char *Config_Script_Command = "bash <(curl -fsSL https://fastly.jsdelivr.net/gh/AllesUgo/Minecraft-Speed-Proxy@master/scripts/config.sh )";
int LocalPort;
int IsOnlinePlayerNumberShow = 0;
pthread_key_t Thread_Key;
int Remote_Port;
char DlCheck = 0;
char *jdata;
pthread_mutex_t Motd_Lock = PTHREAD_MUTEX_INITIALIZER;
void OnlineControl_Init();
void *DealClient(void *InputArg);
void addip(int sock, const char *IP);
void sighandle(int sig);
void printhelp(void);
int ReadConfig(const char *filepath, char **sremoteserveraddress, int *remoteport, int *localport, char *noinput_sign);
void printhelp(void)
{
	printf("minecraftspeedproxy\n\t <远程服务器地址> <远程服务器端口> <本地监听端口> [可选参数]\t启动服务器\n\t--version\t显示版本信息\n");
	printf("\t--help\t获取帮助\n");
	printf("\t-a\t在默认位置%s生成默认配置文件\n", Defalut_Configfile_Path);
	printf("\t-c\t<配置文件路径>\t使用指定的配置文件启动服务器\n");
	printf("\t在系统命令行使用 %s 交互生成配置文件\n", Config_Script_Command);
	printf("\n支持的可选参数(仅可用于通过命令行参数启动服务器方式):\n");
	printf("\t--noinput\t无命令控制\n");
	printf("\t--enable-whitelist\t默认启用白名单\n");
}
void sighandle(int sig)
{
	switch (sig)
	{
	case SIGINT:
		// printf("[%s] 退出服务器\n", gettime().time);
		log_info("退出服务器");
		exit(0);
		break;
	case SIGSEGV:
		// printf("[%s] [E] 服务器内部错误，请重新启动服务进程，您也可以将error.log发送给开发人员\n", gettime().time);
		log_error("服务器内部错误，建议重新启动服务进程，希望您可以将error.log发送给开发人员");
		int ret;
		ret += system("echo ==================>>error.log");
		char str[128];
		sprintf(str, "echo version=%s>>error.log", VERSION);
		ret += system(str);
		ret += system("date >>error.log");
		ret += system("uname -a >>error.log");
		ret += system("echo ulimit: >>error.log");
		ret += system("ulimit -s>>error.log");
		dump_func("error.log");
		ret += system("echo ==================>>error.log");
		if (ret != 0)
		{
			log_error("error.log日志可能保存失败");
		}
		longjmp(*(jmp_buf *)(pthread_getspecific(Thread_Key)), SIGSEGV);
		break;
	case SIGABRT:
		log_error("出现错误，尝试恢复");
		longjmp(*(jmp_buf *)(pthread_getspecific(Thread_Key)), SIGABRT);
		break;
	default:
		break;
	}
}
int main(int argc, char *argv[])
{
	signal(SIGINT, sighandle);
	signal(SIGABRT, sighandle);
	signal(SIGSEGV, sighandle);
	signal(SIGPIPE, SIG_IGN);
	Version = (char *)malloc(1024);
	sprintf(Version, "Version:%s\n编译时间:%s\n编译器版本:%s\n", DVERSION, __DATE__, __VERSION__);
	char noinput_sign = 0;
	if (argc == 1)
	{
		printf("默认加载%s\n", Defalut_Configfile_Path);
		DlCheck = CL_TryLoadDlfcn("./check.so");
		if (0 != ReadConfig(Defalut_Configfile_Path, &remoteServerAddress, &Remote_Port, &LocalPort, &noinput_sign))
		{
			printf("配置文件加载失败，可使用 -a参数来默认生成一个配置文件%s\n", Defalut_Configfile_Path);
			printf("或使用 %s 交互生成配置文件\n", Config_Script_Command);
			printf("使用参数--help以获取使用帮助\n");
			return 1;
		}
		// printf("[%s] [I] 配置文件加载成功，若要获取使用帮助请使用minecraftspeedproxy --help\n",gettime().time);
		log_info("配置文件加载成功，若要获取使用帮助请使用minecraftspeedproxy --help");
	}
	else if (argc == 2)
	{
		if (!strcmp(argv[1], "--version"))
		{
			puts(Version);
			return 0;
		}
		else if (!strcmp(argv[1], "--help"))
		{
			printhelp();
			return 0;
		}
		else if (!strcmp(argv[1], "-a"))
		{
			if (0 != access("/etc/minecraftspeedproxy/", F_OK))
			{
				if (0 != mkdir("/etc/minecraftspeedproxy/", 0777))
				{
					printf("创建目录/etc/minecraftspeedproxy/失败:%s\n", strerror(errno));
					if (errno == 13)
					{
						printf("请检查是否拥有/etc/目录的读写权限\n");
					}

					return 1;
				}
			}
			cJSON *temp = cJSON_CreateObject();
			cJSON_AddStringToObject(temp, "Address", "mc.hypixel.net");
			cJSON_AddNumberToObject(temp, "RemotePort", 25565);
			cJSON_AddNumberToObject(temp, "LocalPort", 25565);
			cJSON_AddBoolToObject(temp, "DefaultEnableWhitelist", cJSON_False);
			cJSON_AddBoolToObject(temp, "AllowInput", cJSON_True);
			cJSON_AddBoolToObject(temp, "ShowOnlinePlayerNumber", cJSON_False);
			char *jsonstr = cJSON_Print(temp);
			cJSON_Delete(temp);
			FILE *fp = fopen(Defalut_Configfile_Path, "w");
			if (fp == NULL)
			{
				printf("无法打开%s,原因:%s\n", Defalut_Configfile_Path, strerror(errno));
				free(jsonstr);
				if (errno == 13)
				{
					printf("请检查是否拥有/etc/minecraftspeedproxy/目录的读写权限\n");
				}
				return 1;
			}
			fprintf(fp, "%s\n", jsonstr);
			free(jsonstr);
			fclose(fp);
			printf("生成配置文件成功:%s\n", Defalut_Configfile_Path);
			return 0;
		}
		else
		{
			printf("参数错误\n");
			printhelp();
			return 1;
		}
	}
	else if (argc < 3)
	{
		printf("参数错误\n");
		printhelp();
		return 1;
	}
	else if (argc == 3)
	{
		if (!strcmp(argv[1], "-c"))
		{
			DlCheck = CL_TryLoadDlfcn("./check.so");
			if (0 != ReadConfig(argv[2], &remoteServerAddress, &Remote_Port, &LocalPort, &noinput_sign))
			{
				return 1;
			}
		}
		else
		{
			printf("不支持的参数%s\n", argv[1]);
			return 1;
		}
	}
	else if (argc >= 4)
	{
		DlCheck = CL_TryLoadDlfcn("./check.so");
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
				log_info("禁用命令行控制");
			}
			else if (!strcmp(argv[i], "--enable-whitelist"))
			{
				if (DlCheck != 1)
				{
					switch (CL_EnableWhiteList())
					{
					case 0:
						log_info("白名单开启成功");
						break;
					default:
						log_error("白名单开启失败");
						break;
					}
				}
				else
				{
					log_info("自定义登录检查组件已加载，忽略参数--enable-whitelist");
				}
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

	FILE *playerLog = fopen("player.log", "a");
	log_add_fp(playerLog, LOG_PLAYER);

	// printf("[%s] [I] PID:%d 远程服务器:%s:%d 本地监听端口%d\n", gettime().time, getpid(), remoteServerAddress, Remote_Port, LocalPort);
	log_info("PID:%d 远程服务器:%s:%d 本地监听端口%d", getpid(), remoteServerAddress, Remote_Port, LocalPort);
	// 读取motd
	//  printf("[%s] [I] 加载motd数据\n", gettime().time);
	log_info("加载motd数据");
	FILE *fp = fopen("motd.json", "r");
	if (fp == NULL)
	{
		// printf("[%s] [W] 没有找到motd.json\n", gettime().time);
		log_warn("没有找到motd.json");
		jdata = (char *)malloc(141);
		strcpy(jdata, "{\"version\": {\"name\": \"1.8.7\",\"protocol\": 47},\"players\": {\"max\": 0,\"online\": 0,\"sample\": []},\"description\": {\"text\": \"Minecraft Speed Plus\"}}");
		fp = fopen("motd.json", "w");
		if (fp == NULL)
		{
			// printf("[%s] [W] 无法保存motd.json文件，原因是:%s\n", gettime().time, strerror(errno));
			log_warn("无法保存motd.json文件，原因是:%s", strerror(errno));
		}
		else
		{
			fputs(jdata, fp);
			fclose(fp);
			// printf("[%s] [I] 已生成默认的motd.json\n", gettime().time);
			log_info("已生成默认的motd.json\n");
		}
	}
	else
	{
		struct stat st;
		if (stat("motd.json", &st))
		{
			log_error("获取motd.json文件信息失败\n");
			exit(1);
		}
		jdata = (char *)malloc(st.st_size);
		if (1 != fread(jdata, st.st_size, 1, fp))
		{
			log_warn("读取motd.json异常\n");
		}
		fclose(fp);
	}
	if (noinput_sign == 0)
	{
		// printf("[%s] [I] 初始化在线人数管理\n", gettime().time);
		log_info("初始化在线人数管理");
		OnlineControl_Init();
	}
	// 创建监听端口
	//  printf("[%s] [I] 初始化服务端口\n", gettime().time);
	log_info("初始化服务端口");
	WS_ServerPort_t server = WS_CreateServerPort(LocalPort, 5);
	if (server == 0)
	{
		if (errno == 98)
		{
			log_error("绑定端口%d失败，端口已被占用", LocalPort);
		}
		else if (errno == 13)
		{
			log_error("绑定端口%d失败，请检查是否有权限绑定该端口，使用root权限运行本程序可能会解决该问题", LocalPort);
		}

		fclose(playerLog);
		return 1;
	}
	// 初始化线程专享空间
	pthread_key_create(&Thread_Key, NULL);
	// 循环等待用户连接
	WS_Connection_t *client;
	pthread_t pid;
	// printf("[%s] [I] 加载完成，等待连接，输入help获取帮助\n", gettime().time);
	log_info("加载完成，等待连接，输入help获取帮助");
	while (1)
	{
		client = (WS_Connection_t *)malloc(sizeof(WS_Connection_t));
		if (-1 == WS_WaitClient(server, client))
		{
			// 建立连接失败
			//  printf("[%s] [W] 连接建立失败:%s\n", gettime().time, strerror(errno));
			log_warn("连接建立失败:%s", strerror(errno));
			free(client);
			continue;
		}
		addip(client->sock, client->addr);
		if (0 != pthread_create(&pid, NULL, DealClient, client))
		{
			// printf("[%s] [E] 创建线程失败\n", gettime().time);
			log_error("创建线程失败");
			WS_CloseConnection(client);
			free(client);
			sleep(5);
			continue;
		}
		pthread_detach(pid);
	}
	fclose(playerLog);
}
int ReadConfig(const char *filepath, char **remoteserveraddress, int *remoteport, int *localport, char *noinput_sign)
{
	FILE *fp = fopen(filepath, "r");
	if (fp == NULL)
	{
		// printf("无法打开文件%s,原因:%s\n", filepath, strerror(errno));
		log_warn("无法打开文件%s,原因:%s", filepath, strerror(errno));
		return -1;
	}
	// 获取文件大小
	fseek(fp, 0L, SEEK_END);
	size_t filesize = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	if (filesize == 0 || filesize > 1024 * 1024)
	{
		// printf("配置文件大小错误:%lu\n", filesize);
		log_warn("配置文件大小错误:%lu", filesize);
		fclose(fp);
		return -2;
	}
	char *jsondata = (char *)malloc(filesize);
	if (1 != fread(jsondata, filesize, 1, fp))
	{
		// printf("读取配置文件错误\n");
		log_warn("读取配置文件错误");
		fclose(fp);
		free(jsondata);
		return -3;
	}
	fclose(fp);
	cJSON *json = cJSON_Parse(jsondata);
	free(jsondata);
	cJSON *temp = cJSON_GetObjectItem(json, "Address");
	if (temp == NULL || temp->type != cJSON_String)
	{
		// printf("配置文件错误，不存在Address或其键值没有使用字符串\n");
		log_warn("配置文件错误，不存在Address或其键值没有使用字符串");
		cJSON_Delete(json);
		return -4;
	}
	*remoteserveraddress = (char *)malloc(strlen(temp->valuestring) + 1);
	strcpy(*remoteserveraddress, temp->valuestring);
	temp = cJSON_GetObjectItem(json, "RemotePort");
	if (temp == NULL || temp->type != cJSON_Number)
	{
		// printf("配置文件错误，不存在RemotePort或其键值没有使用整数,默认使用25565端口\n");
		log_warn("配置文件错误，不存在RemotePort或其键值没有使用整数,默认使用25565端口");
		*remoteport = 25565;
	}
	else
	{
		*remoteport = temp->valueint;
	}
	temp = cJSON_GetObjectItem(json, "LocalPort");
	if (temp == NULL || temp->type != cJSON_Number)
	{
		// printf("配置文件错误，不存在LocalPort或其键值没有使用整数,默认使用25565端口\n");
		log_warn("配置文件错误，不存在LocalPort或其键值没有使用整数,默认使用25565端口");
		*localport = 25565;
	}
	else
	{
		*localport = temp->valueint;
	}
	temp = cJSON_GetObjectItem(json, "AllowInput");
	if (temp == NULL || (temp->type != cJSON_True && temp->type != cJSON_False))
	{
		// printf("配置文件错误，不存在AllowInput或其键值没有使用布尔值,默认开启命令行控制\n");
		log_warn("配置文件错误，不存在AllowInput或其键值没有使用布尔值,默认开启命令行控制");
		*noinput_sign = 0;
	}
	else
	{
		if (temp->type == cJSON_False)
		{
			*noinput_sign = 1;
			// printf("禁用命令行控制\n");
			log_info("禁用命令行控制");
		}
	}
	temp = cJSON_GetObjectItem(json, "DefaultEnableWhitelist");
	if (temp == NULL || (temp->type != cJSON_True && temp->type != cJSON_False))
	{
		// printf("配置文件错误，不存在AllowInput或其键值没有使用布尔值,默认开启命令行控制\n");
		log_warn("配置文件错误，不存在DefaultEnableWhitelist或其键值没有使用布尔值,默认关闭白名单功能");
	}
	else
	{
		if (DlCheck != 1)
		{
			if (temp->type == cJSON_True)
			{
				switch (CL_EnableWhiteList())
				{
				case 0:
					log_info("白名单开启成功");
					break;
				default:
					log_error("白名单开启失败");
					break;
				}
			}
			else
				log_info("白名单默认关闭");
		}
		else
		{
			log_info("使用自定义组件检查登录，忽略内置白名单初始化");
		}
	}
	temp = cJSON_GetObjectItem(json, "ShowOnlinePlayerNumber");
	if (temp == NULL || (temp->type != cJSON_True && temp->type != cJSON_False))
	{
		IsOnlinePlayerNumberShow = 0;
		log_warn("配置文件中没有ShowOnlinePlayerNumber项或该项不为布尔类型，默认关闭客户端在线人数显示");
	}
	else
	{
		if (temp->type == cJSON_True)
		{
			IsOnlinePlayerNumberShow = 1;
			log_info("客户端在线人数显示已开启");
		}
		else
		{
			IsOnlinePlayerNumberShow = 0;
			log_info("客户端在线人数显示已关闭");
		}
	}
	cJSON_Delete(json);
	return 0;
}
