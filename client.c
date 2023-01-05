#include <stdio.h>
#include <pthread.h>
#include <setjmp.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include "log.h"
#include "websocket.h"
#include "RemoteClient.h"
#include "unpack.h"
#include "cJSON.h"
#include <alloca.h>
#include <netinet/tcp.h>
#include "CheckLogin.h"

extern char *remoteServerAddress;
extern int LocalPort;
extern int Remote_Port;
extern char *jdata;
extern pthread_key_t Thread_Key;

void *DealClient(void *InputArg);
void adduser(int sock, const char *username);
void *DealRemote(void *InputArg);
void removeip(int sock);
int SendResponse(WS_Connection_t client, char *jdata, int pro);
int SendPong(WS_Connection_t client, char *data, int datasize);
// int chick(char *name);
int CL_Check(const char *username);
void SendKick(WS_Connection_t client, const char *reason);

void *SendingThread(void *pack);
SendingPack_t InitSending(int target_sock, int maxdata);
DataLink_t *UpSendingData(SendingPack_t *pack, DataLink_t *link, void *data, size_t size);
void printdata(char *data, int datasize)
{
    printf("\n");
    for (int i = 0; i < datasize; i++)
    {
        printf("%x ", data[i]);
    }
    printf("\n");
}

void *DealClient(void *InputArg)
{
    //设置线程独享资源
    jmp_buf jmp;
    pthread_setspecific(Thread_Key, &jmp);
    //接收多线程传参
    WS_Connection_t client = *((WS_Connection_t *)InputArg);
    free(InputArg);
    WS_Connection_t remoteserver;

    int opt = 6, err = 0;
    // err += setsockopt(remoteserver.sock, SOL_SOCKET, SO_PRIORITY, &opt, sizeof(opt)); /*设置s的优先级*/
    err += setsockopt(client.sock, SOL_SOCKET, SO_PRIORITY, &opt, sizeof(opt)); /*设置s的优先级*/
    if (err != 0)
    {
        // printf("[%s] [W] 优先级设置失败：%s\n", gettime().time, strerror(errno));
        log_warn("优先级设置失败：%s", strerror(errno));
    }
    struct timeval timeout = {10, 0}; //设置10s超时
    setsockopt(client.sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(remoteserver.sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    //将双方服务器套接字打包
    Rcpack *pack = (Rcpack *)malloc(sizeof(Rcpack));
    // char remotedata[8192];
    //拉起与服务端连接的线程
    pthread_t pid;
    //进入循环接收客户端数据数据
    int rsnum;       //收发的数据量
    char data[8192]; //数据包
    //首先接收一个包并且看包ID
    char handshaked = 0;
    char logined = 0;
    int statusmode = 0;
    char *handpackdata = (char *)malloc(4096);
    HandPack hp;
    void *stackdata;
    switch (setjmp(jmp))
    {
    case SIGABRT:
        log_error("线程异常，已放弃");
        goto CLOSECONNECT;
        break;
    case SIGSEGV:
        log_error("线程段错误，尝试恢复，本恢复可能造成部分资源无法完全释放");
        goto CLOSECONNECT;
        break;
    default:
        break;
    }
    while (1)
    {

        if (logined == 0)
        {
            //还没有登录
            if (handshaked == 0)
            {
                //还没有握手,先握手
                memset(data, 0, 4096);

                int handshakepacksize = RecvFullPack(client, data, 4096);

                if (handshakepacksize <= 0)
                {
                    // printf("[%s] [%lu] [W] 握手包错误，无法完成握手\n", gettime().time, pthread_self());
                    log_warn("[%lu] 握手包错误，无法完成握手", pthread_self());
                    goto CLOSECONNECT;
                }
                if (GetPackID(data, handshakepacksize) != 0)
                {
                    // printf("[%s] [%lu] [W] 不是握手包，无法完成握手\n", gettime().time, pthread_self());
                    log_warn("[%lu] 不是握手包，无法完成握手", pthread_self());
                    goto CLOSECONNECT;
                }
                if (0 != ParseHandlePack(&hp, data, handshakepacksize))
                {
                    // printf("[%s] [%lu] [W] 握手包无法解析\n", gettime().time, pthread_self());
                    log_warn("[%lu] 握手包无法解析", pthread_self());
                    goto CLOSECONNECT;
                }
                // printf("[%s] [I] %s连接到服务器\n", gettime().time, client.addr);
                log_info("%s连接到服务器", client.addr);
                if (hp.nextstate == 1)
                {
                    statusmode = 1; // status连接
                }
                else
                {
                    statusmode = 0; // login连接
                }
                handshaked = 1; //将连接设置为已握手状态
                continue;
            }
            else
            {
                //已握手，但是并没有完成登录，可能是status或者还没有login
                memset(data, 0, 4096);
                int datasize = RecvFullPack(client, data, 4096);
                if (datasize <= 0)
                {
                    goto CLOSECONNECT;
                }
                switch (GetPackID(data, datasize))
                {
                case -10000:
                    goto CLOSECONNECT;
                    break;
                case 0:
                    //登录或者request包
                    if (statusmode == 1)
                    {
                        // request包
                        SendResponse(client, jdata, hp.protocol);
                        continue;
                    }
                    else
                    {
                        //登录包
                        unsigned char vil;
                        varint_decode(data, datasize, &vil);
                        char username[20];
                        ReadString(data + vil + 1, datasize - vil, username, 20);
                        switch (CL_Check(username))
                        {
                        case CHECK_LOGIN_SUCCESS:
                        {
                            char message[128];
                            int packsize = BuildHandPack(hp, remoteServerAddress, message, 128);
                            if (0 != WS_ConnectServer(remoteServerAddress, Remote_Port, &remoteserver))
                            {
                                // printf("[%s] [E] 无法连接远程服务器，原因是%s\n", gettime().time, strerror(errno));
                                log_error("无法连接远程服务器，原因是%s", strerror(errno));
                                WS_CloseConnection(&client);
                                removeip(client.sock);
                                free(handpackdata);
                                free(pack);
                                return NULL;
                            }
                            err += setsockopt(remoteserver.sock, SOL_SOCKET, SO_PRIORITY, &opt, sizeof(opt)); /*设置s的优先级*/
                            if (err != 0)
                            {
                                // printf("[%s] [W] 优先级设置失败：%s\n", gettime().time, strerror(errno));
                                log_warn("优先级设置失败：%s", strerror(errno));
                            }
                            pack->client = client;
                            pack->server = remoteserver;
                            pthread_create(&pid, NULL, DealRemote, pack);
                            strcpy(hp.Address, remoteServerAddress);
                            hp.port = Remote_Port;
                            hp.nextstate = 2;
                            WS_Send(&remoteserver, message, packsize);
                            WS_Send(&remoteserver, data, datasize);
                            adduser(client.sock, username);
                            goto ACCEPTWHILE;
                        }
                        case CHECK_LOGIN_BANNED:
                            SendKick(client, "您已被加速服务器封禁");
                            goto CLOSECONNECT;
                        case CHECk_LOGIN_NOT_ON_WHITELIST:
                            SendKick(client, "服务器已开启白名单");
                            goto CLOSECONNECT;
                        default:
                            SendKick(client, "代理服务器拒绝连接");
                            goto CLOSECONNECT;
                        }
                    }
                case 1:
                    // ping包
                    SendPong(client, data, datasize);
                    continue;
                default:
                    continue;
                }
            }
        }

    ACCEPTWHILE:
        stackdata = malloc(512);
        while (1)
        {
            
            rsnum = read(client.sock, stackdata, 512);
            if (rsnum <= 0)
            {
                free(stackdata);
                shutdown(client.sock, SHUT_RDWR);
                shutdown(remoteserver.sock, SHUT_RDWR);
                break;
            }
            send(remoteserver.sock,stackdata,rsnum,MSG_DONTWAIT);
        }
        pthread_join(pid, NULL);           //等待服务线程资源回收
        WS_CloseConnection(&remoteserver);
    }
CLOSECONNECT:
    //断开连接并且回收资源
    WS_CloseConnection(&client);
    free(handpackdata);
    free(pack);
    removeip(client.sock);
    // printf("[%s] [I] %s断开连接\n", gettime().time, client.addr);
    log_info("%s断开连接", client.addr);
    return NULL;
}

int SendPong(WS_Connection_t client, char *data, int datasize)
{
    return WS_Send(&client, data, datasize);
}

int SendResponse(WS_Connection_t client, char *jdata, int pro)
{

    cJSON *json = cJSON_Parse(jdata);
    if (json == NULL)
    {
        log_warn("motd.json数据格式错误");
        return -3;
    }
    cJSON *version = cJSON_GetObjectItem(json, "version");
    if (version == NULL)
    {
        cJSON_Delete(json);
        //数据格式错误!
        // printf("[%s] [W] motd.json数据格式错误\n", gettime().time);
        log_warn("motd.json数据格式错误");
        return -3;
    }
    cJSON *temp = cJSON_CreateNumber(pro);
    cJSON_ReplaceItemInObject(version, "protocol", temp);
    char *jjdata = cJSON_Print(json);
    cJSON_Delete(json);
    char *data = (char *)malloc(strlen(jjdata) + 32);
    int datapacksize = BuildString(jjdata, data, strlen(jjdata) + 32);
    if (datapacksize <= 0)
    {
        free(jjdata);
        return -1;
    }
    unsigned char vil;
    char vi[10];
    varint_encode(datapacksize + 1, vi, 10, &vil);
    char *message = (char *)malloc(strlen(jjdata) + 32);
    free(jjdata);
    char *p = message;
    memcpy(message, vi, vil);
    p += vil;
    *p = 0;
    p += 1;
    memcpy(p, data, datapacksize);
    free(data);
    int sendnum = WS_Send(&client, message, datapacksize + vil + 1);
    if (sendnum <= 0)
    {
        free(message);
        return -2;
    }
    free(message);
    return 0;
}
void SendKick(WS_Connection_t client, const char *reason)
{
    //创建json包

    char *str = (char *)malloc(strlen(reason) + 10);
    sprintf(str, "\"%s\"", reason);

    char *data = (char *)malloc(strlen(str) + 20);
    int strl = BuildString(str, data, strlen(str) + 20);
    char *message = (char *)malloc(strlen(str) + 30);
    unsigned char vil;
    char vi[10];
    varint_encode(strl + 1, vi, 10, &vil);
    memcpy(message, vi, vil);
    char *p = message + vil;
    *p = 0;
    p += 1;
    memcpy(p, data, strl);
    WS_Send(&client, message, strl + vil + 1);
    free(data);
    free(str);
    free(message);
}
