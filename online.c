#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "cJSON.h"
#include "CheckLogin.h"
#include "log.h"
#include "websocket.h"

typedef struct ONLINE_LINK
{
    int sock;
    char ip[32];
    char username[32];
    pthread_t clientpid;
    pthread_t serverpid;
    time_t starttime;
    struct ONLINE_LINK *next;
} OL_L;
struct ARGS_STRUCT
{
    unsigned int argc;
    char **argv;
};
pthread_mutex_t O_lock;
OL_L *head;
extern char *Version;
extern int IsOnlinePlayerNumberShow;
extern pthread_mutex_t Motd_Lock;
extern char *jdata;
int OnlineNumber = 0;
void printonline();
void kickplayer(const char *playername);
void kicksock(int sock);
void MotdControl(struct ARGS_STRUCT *args);
int GetOnlinePlayerNumber();
struct ARGS_STRUCT ParseInputStr(const char *str)
{
    char *buffer = (char *)malloc(strlen(str) + 1);
    char **argv = (char **)malloc((strlen(str) + 1) * sizeof(char **));
    int argc = 0;
    for (int i = 0; str[i] != 0; i++)
    {
        if (str[i] != ' ' && str[i] != '\n' && str[i] != '\r')
        {
            int k = 0;
            for (; str[i] != 0 && str[i] != ' ' && str[i] != '\n' && str[i] != '\r'; i++, k++)
            {
                buffer[k] = str[i];
            }
            buffer[k] = 0;
            char *temp = (char *)malloc(strlen(buffer) + 1);
            strcpy(temp, buffer);
            argv[argc] = temp;
            argc++;
            i--;
        }
    }
    struct ARGS_STRUCT out;
    free(buffer);
    out.argc = argc;
    out.argv = argv;
    out.argv = (char **)malloc(sizeof(char **) * out.argc);
    memcpy(out.argv, argv, sizeof(char **) * out.argc);
    free(argv);
    return out;
}
void DeleteArgsStruct(struct ARGS_STRUCT *in)
{
    for (int i = 0; i < in->argc; i++)
    {
        free((in->argv)[i]);
    }
    if (in->argv != NULL)
        free(in->argv);
}
void *thread(void *a)
{
    char *inputstr = (char *)malloc(1024 * 1024);
    char *cmd = (char *)malloc(1024);
    char *ptr;
    while (1)
    {
        struct ARGS_STRUCT input_struct = {0};
        printf(">>");
        DeleteArgsStruct(&input_struct);
        fgets(inputstr, 1024 * 1024, stdin);
        input_struct = ParseInputStr(inputstr);
        ptr = inputstr;
        if (sscanf(inputstr, "%s", cmd) <= 0)
            continue;
        ptr += strlen(cmd);
        if (!strcmp(cmd, "list"))
        {
            printonline();
        }
        else if (!strcmp(cmd, "help"))
        {
            printf("命令列表:\nhelp\t查看帮助\nstop\t退出程序\nmemuse\t查看内存使用情况\nlist\t列出在线玩家\npid\t获取当前进程pid\nkick --help\t获取kick命令帮助\nwhitelist --help\t获取白名单命令帮助\nban --help\t获取封禁命令帮助\npardon --help\t获取接触封禁命令帮助\nOnlinePlayerNumber --help\t获取在线玩家展示设置帮助\n");
            printf("motd --help\t获取管理motd状态的帮助\n");
        }
        else if (!strcmp(cmd, "version"))
        {
            puts(Version);
        }
        else if (!strcmp(cmd, "pid"))
        {
            printf("当前进程的PID是%d\n", getpid());
        }
        else if (!strcmp(cmd, "stop"))
        {
            exit(0);
        }
        else if (!strcmp(cmd, "memuse"))
        {
            printf(">>进程内存使用信息<<\n");
            int info = open("/proc/self/status", O_RDONLY);
            char text[1024];
            read(info, text, 1024);
            // printf("%s\n",text);
            // 打印内存峰值
            char *p = strstr(text, "VmPeak");
            printf("内存使用峰值:");
            for (int i = 0; p[i] != '\n' && p != NULL; i++)
            {
                putc(p[i], stdout);
            }
            putc('\n', stdout);
            // 打印当前内存使用量
            p = strstr(text, "VmSize:");
            printf("内存使用量:");
            for (int i = 0; p[i] != '\n' && p != NULL; i++)
            {
                putc(p[i], stdout);
            }
            putc('\n', stdout);

            // 打印当前物理内存使用量
            p = strstr(text, "VmRSS:");
            printf("物理内存使用量:");
            for (int i = 0; p[i] != '\n' && p != NULL; i++)
            {
                putc(p[i], stdout);
            }
            putc('\n', stdout);

            // 打印栈大小
            p = strstr(text, "VmStk:");
            printf("栈内存使用量:");
            for (int i = 0; p[i] != '\n' && p != NULL; i++)
            {
                putc(p[i], stdout);
            }
            putc('\n', stdout);

            close(info);
            printf(">>信息显示完成<<\n");
        }
        else if (!strcmp(cmd, "pardon"))
        {
            cmd[0] = 0; // 重置字符串
            ptr += strlen(cmd);
            while (*ptr == ' ')
                ptr += 1;
            if (0 >= sscanf(ptr, "%s", cmd))
                goto PARDONCMDERROR;
            ptr += strlen(cmd);
            while (*ptr == ' ')
                ptr += 1;

            if (!strcmp("--help", cmd))
            {
                printf("\n\npardon <playername>\t解除封禁玩家\n");
            }
            else
            {
                switch (CL_BanListRemove(cmd))
                {
                case -1:
                    printf("该玩家未被封禁\n");
                    break;

                default:
                    printf("已解除封禁%s\n", cmd);
                    break;
                }
            }
            continue;
        PARDONCMDERROR:
            printf("\n参数错误，使用pardon --help查看帮助\n");
        }
        else if (!strcmp(cmd, "ban"))
        {
            cmd[0] = 0; // 重置字符串
            ptr += strlen(cmd);
            while (*ptr == ' ')
                ptr += 1;
            if (0 >= sscanf(ptr, "%s", cmd))
                goto BANCMDERROR;
            ptr += strlen(cmd);
            while (*ptr == ' ')
                ptr += 1;

            if (!strcmp("--help", cmd))
            {
                printf("\n\nban <playername>\t封禁玩家\n");
            }
            else
            {
                switch (CL_BanListAdd(cmd))
                {
                case -1:
                    printf("该玩家已经被封禁，无需重复封禁\n");
                    break;

                default:
                    printf("已封禁%s,若要将%s立即踢出服务器请使用kick %s,解除封禁请使用pardon命令\n", cmd, cmd, cmd);
                    break;
                }
            }
            continue;
        BANCMDERROR:
            printf("\n参数错误，使用ban --help查看帮助\n");
        }
        else if (!strcmp(cmd, "whitelist"))
        {
            cmd[0] = 0; // 重置字符串
            ptr += strlen(cmd);
            while (*ptr == ' ')
                ptr += 1;
            if (0 >= sscanf(ptr, "%s", cmd))
                goto WHITELISTCMDERROR;
            ptr += strlen(cmd);
            while (*ptr == ' ')
                ptr += 1;
            if (!strcmp("on", cmd))
            {
                switch (CL_EnableWhiteList())
                {
                case -2:
                    printf("白名单已开启，无需重新开启\n");
                    break;
                case -1:
                    break;
                default:
                    printf("白名单开启成功\n");
                    break;
                }
            }
            else if (!strcmp("off", cmd))
            {
                switch (CL_DisabledWhiteList())
                {
                case -2:
                    printf("白名单已关闭，无需重复关闭\n");
                    break;
                default:
                    printf("白名单关闭成功\n");
                    break;
                }
            }
            else if (!strcmp("add", cmd))
            {
                cmd[0] = 0;
                if (sscanf(ptr, "%s", cmd) <= 0)
                    goto WHITELISTCMDERROR;
                switch (CL_WhiteListAdd(cmd))
                {
                case -2:
                    printf("白名单未开启\n");
                    break;
                case -1:
                    printf("玩家已在白名单,无需重复添加\n");
                    break;
                default:
                    printf("成功将%s添加到白名单\n", cmd);
                    break;
                }
            }
            else if (!strcmp("remove", cmd))
            {
                cmd[0] = 0;
                if (sscanf(ptr, "%s", cmd) <= 0)
                    goto WHITELISTCMDERROR;
                switch (CL_WhiteListRemove(cmd))
                {
                case -2:
                    printf("白名单未开启\n");
                    break;
                case -1:
                    printf("白名单中没有该玩家\n");
                    break;
                default:
                    printf("已将%s从白名单中移除\n", cmd);
                    break;
                }
            }
            else if (!strcmp("reload", cmd))
            {
                if (0 != CL_ReloadWhiteList())
                {
                    printf("重新加载白名单文件失败\n");
                }
            }
            else if (!strcmp("--help", cmd))
            {
                printf("\n\nwhitelist <on/off/reload>\t开/关/重新加载白名单\n");
                printf("whitelist <add/remove> <playername>\t向白名单添加/移除用户\n");
            }

            else
            {
            WHITELISTCMDERROR:
                printf("\n参数错误，使用whitelist --help查看帮助\n");
            }
        }
        else if (!strcmp(cmd, "kick"))
        {
            cmd[0] = 0; // 重置字符串
            ptr += strlen(cmd);
            while (*ptr == ' ')
                ptr += 1;
            if (0 >= sscanf(ptr, "%s", cmd))
                goto KICKPLAYERCMDERROR;
            ptr += strlen(cmd);
            while (*ptr == ' ')
                ptr += 1;
            if (!strcmp("player", cmd))
            {
                cmd[0] = 0;
                if (sscanf(ptr, "%s", cmd) <= 0)
                    goto KICKPLAYERCMDERROR;
                kickplayer(cmd);
            }
            else if (!strcmp("sock", cmd))
            {
                int sock = 0;
                if (sscanf(ptr, "%d", &sock) <= 0)
                    goto KICKPLAYERCMDERROR;
                kicksock(sock);
            }
            else if (!strcmp("--help", cmd))
            {
                printf("\n\nkick player [playername]\t通过用户名踢出用户\n");
                printf("kick sock [sock]\t通过套接字踢出用户，套接字可根据list命令获得\n");
            }
            else
            {
            KICKPLAYERCMDERROR:
                printf("\n参数错误,使用kick --help获取帮助\n");
            }
        }
        else if (!strcmp("OnlinePlayerNumber", input_struct.argv[0]))
        {
            if (input_struct.argc != 2)
            {
                printf("参数错误，请输入OnlinePlayerNumber --help以获取帮助\n");
            }
            else
            {
                if (!strcmp("enable", input_struct.argv[1]))
                {
                    if (IsOnlinePlayerNumberShow == 1)
                    {
                        printf("当前已开启，无需重复开启\n");
                    }
                    else
                    {
                        IsOnlinePlayerNumberShow = 1;
                        printf("开启成功\n");
                    }
                }
                else if (!strcmp("disable", input_struct.argv[1]))
                {
                    if (IsOnlinePlayerNumberShow == 0)
                    {
                        printf("当前已关闭，无需重复关闭\n");
                    }
                    else
                    {
                        IsOnlinePlayerNumberShow = 0;
                        printf("关闭成功\n");
                    }
                }
                else if (!strcmp("--help", input_struct.argv[1]))
                {
                    printf("用于控制在客户端是否显示在线玩家人数\n");
                    printf("输入OnlinePlayerNumber enable以开启显示\n");
                    printf("输入OnlinePlayerNumber disable以关闭\n");
                }
                else
                {
                    printf("参数错误，请输入OnlinePlayerNumber help以获取帮助\n");
                }
            }
        }
        else if (!strcmp(input_struct.argv[0], "motd"))
        {
            MotdControl(&input_struct);
        }
        else
        {
            printf("未知命令，可输入help查看帮助\n");
        }
    }
}
void kicksock(int sock)
{
    pthread_mutex_lock(&O_lock);
    OL_L *temp = head;
    char successinfo = 0;
    while (temp != NULL)
    {
        if (sock == temp->sock)
        {
            successinfo = 1;

            if (0 == shutdown(temp->sock, SHUT_RDWR))
            {
                printf("已关闭IP=%s,username=%s玩家的连接\n", temp->ip, temp->username);
            }
            else
            {
                printf("未能成功关闭IP=%s,username=%s玩家的连接:%s\n", temp->ip, temp->username, strerror(errno));
            }
        }
        temp = temp->next;
    }
    if (successinfo == 0)
    {
        printf("未找到%d的连接\n", sock);
    }
    pthread_mutex_unlock(&O_lock);
}
void kickplayer(const char *playername)
{
    pthread_mutex_lock(&O_lock);
    OL_L *temp = head;
    char successinfo = 0;
    while (temp != NULL)
    {
        if (!strcmp(playername, temp->username))
        {
            successinfo = 1;
            if (0 == shutdown(temp->sock, SHUT_RDWR))
            {
                printf("已关闭IP=%s,username=%s玩家的连接\n", temp->ip, temp->username);
            }
            else
            {
                printf("未能成功关闭IP=%s,username=%s玩家的连接:%s\n", temp->ip, temp->username, strerror(errno));
            }
        }
        temp = temp->next;
    }
    if (successinfo == 0)
    {
        printf("未找到%s的连接\n", playername);
    }
    pthread_mutex_unlock(&O_lock);
}
void OnlineControl_Init()
{
    // 加载封禁列表
    if (0 != CL_LoadBanList())
    {
        log_info("封禁玩家列表加载成功");
    }
    pthread_t pid;
    pthread_create(&pid, NULL, thread, NULL);
    pthread_detach(pid);
}
void search(const char *username)
{
    FILE *fp = fopen("whitelist.json", "r");
    if (fp == NULL)
    {
        printf("打开白名单列表失败，原因是:%s\n", strerror(errno));
        return;
    }
    fseek(fp, 0L, SEEK_END);
    int filesize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    char *jdata = (char *)malloc(filesize);
    if (jdata == NULL)
    {
        printf("分配内存失败\n");
        fclose(fp);
        return;
    }
    fread(jdata, filesize, 1, fp);
    fclose(fp);
    cJSON *json = cJSON_Parse(jdata);
    free(jdata);
    if (json == NULL)
    {
        printf("解析白名单文件失败\n");
        return;
    }
    cJSON *person = cJSON_GetObjectItem(json, username);
    if (person == NULL)
    {
        printf("没有找到该条数据\n");
        cJSON_Delete(json);
        return;
    }
    printf("用户名:%s\n", username);
    printf("剩余时间:%ld\n", cJSON_GetObjectItem(person, "EndTime")->valueint - time(NULL));
    printf("封禁状态:%d\n", cJSON_GetObjectItem(person, "IsBan")->valueint);
    printf("该条目更新时间:%s\n", ctime((time_t *)(&(cJSON_GetObjectItem(person, "ChangeTime")->valueint))));
    cJSON_Delete(json);
    return;
}
void printonline()
{
    printf("==========开始列出在线玩家==========\n");

    pthread_mutex_lock(&O_lock);
    OL_L *temp = head;
    char str[20];
    printf("%-5s%-5s%-16s%-16s%s\n", "ID", "sock", "IP", "UserName", "LoginTime");
    int i = 1;
    while (temp)
    {
        printf("%-5d%-5d%-16s%-16s%s", i, temp->sock, temp->ip, temp->username, ctime(&(temp->starttime)));
        temp = temp->next;
        i += 1;
    }
    pthread_mutex_unlock(&O_lock);
    printf("==========列出在线玩家完成==========\n");
}

void removeip(int sock)
{
    pthread_mutex_lock(&O_lock);
    if (head == NULL)
    {
        // 当前没有在线玩家
        pthread_mutex_unlock(&O_lock);
        // printf("[W] 试图移除非在线玩家\n");
        log_warn("试图移除非在线玩家");
        return;
    }
    else
    {
        OL_L *temp = head;
        if (temp->sock == sock)
        {

            if (strcmp(temp->username, "Not-Login") != 0)
            {
                OnlineNumber -= 1;
                log_player("移除了玩家:%s于IP:%s的连接", temp->username, temp->ip);
            }
            head = temp->next;
            free(temp);

            pthread_mutex_unlock(&O_lock);
            // if temp->username != "Not-Login" log
            return;
        }
        while (temp->next)
        {
            if (temp->next->sock == sock)
            {

                OL_L *a = temp->next;
                if (strcmp(temp->username, "Not-Login") != 0)
                {
                    OnlineNumber -= 1;
                    log_player("移除了玩家:%s于IP:%s的连接", a->username, a->ip);
                }
                temp->next = a->next;
                free(a);
                pthread_mutex_unlock(&O_lock);
                return;
            }
            temp = temp->next;
        }
        pthread_mutex_unlock(&O_lock);
        return;
    }
}
void adduser(int sock, const char *username)
{
    pthread_mutex_lock(&O_lock);

    OL_L *temp = head;
    while (temp)
    {
        if (temp->sock == sock)
        {
            strcpy(temp->username, username);
            OnlineNumber += 1;
            log_player("玩家%s登陆于IP:%s的连接", username, temp->ip);
            pthread_mutex_unlock(&O_lock);
            return;
        }
        temp = temp->next;
    }
    ////////////////
    pthread_mutex_unlock(&O_lock);
    return;
}
void addip(int sock, const char *IP)
{
    pthread_mutex_lock(&O_lock);
    if (head == NULL)
    {
        // 当前没有在线玩家
        head = (OL_L *)malloc(sizeof(OL_L));
        strcpy(head->ip, IP);
        strcpy(head->username, "Not-Login");
        head->starttime = time(NULL);
        head->sock = sock;
        head->next = NULL;
        pthread_mutex_unlock(&O_lock);
        return;
    }
    else
    {
        OL_L *temp = head;
        while (temp)
        {
            if (temp->sock == sock)
            {
                printf("[W] 重复的套接字添加,sock=%d,IP=%s\n", sock, IP);
                pthread_mutex_unlock(&O_lock);
                return;
            }
            temp = temp->next;
        }
        temp = head;
        temp = (OL_L *)malloc(sizeof(OL_L));
        strcpy(temp->ip, IP);
        strcpy(temp->username, "Not-Login");
        temp->sock = sock;
        temp->starttime = time(NULL);

        temp->next = head;
        head = temp;
        pthread_mutex_unlock(&O_lock);
        return;
    }
}

int GetOnlinePlayerNumber()
{
    return OnlineNumber;
}
void MotdControl(struct ARGS_STRUCT *args)
{
    struct stat st;
    char *jsondata;
    FILE *fp;
    cJSON*json_test;
    const char*error_reason_str;
    if (args->argc == 2 && !strcmp(args->argv[1], "reload"))
    {
        pthread_mutex_lock(&Motd_Lock);
        do
        {
            if (stat("motd.json", &st))
            {
                log_error("获取motd.json文件状态失败:%s", strerror(errno));
                break;
            }
            jsondata = (char *)malloc(st.st_size + 1);
            if (!(fp = fopen("motd.json", "r")))
            {
                log_error("打开motd.json文件失败:%s", strerror(errno));
                free(jsondata);
                break;
            }
            if (1 != fread(jsondata, st.st_size, 1, fp))
            {
                log_error("读取motd.json文件异常:%s", strerror(errno));
                free(jsondata);
                fclose(fp);
                break;
            }
            jsondata[st.st_size] = 0;
            fclose(fp);
            /*测试motd是否可以被解析*/
            if (!(json_test=cJSON_Parse(jsondata)))
            {
                error_reason_str = cJSON_GetErrorPtr();
                if (error_reason_str) log_error("motd文件JSON解析失败:%s",error_reason_str);
                free(jsondata);
                break;
            }
            cJSON_Delete(json_test);
            free(jdata);
            jdata=jsondata;
            log_info("motd重新加载成功");
        } while (0);
        pthread_mutex_unlock(&Motd_Lock);
    }
    else
    {
        printf("motd语法错误:motd reload重新读取motd文件\n");
    }
}
