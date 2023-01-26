#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <setjmp.h>
#include <fcntl.h>
#include <unistd.h>
#include "log.h"
#include "websocket.h"
#include "RemoteClient.h"

extern char *remoteServerAddress;
extern int LocalPort;
extern int Remote_Port;
extern pthread_key_t Thread_Key;

void *SendingThread(void *pack);
SendingPack_t InitSending(int target_sock, int maxdata);
DataLink_t *UpSendingData(SendingPack_t *pack, DataLink_t *link, void *data, size_t size);

void *DealRemote(void *InputArg);
void *DealRemote(void *InputArg)
{
    // 设置线程独享资源
    int fd[2];
    jmp_buf jmp;
    pthread_setspecific(Thread_Key, &jmp);
    // 接收多线程传参
    Rcpack *pack = ((Rcpack *)InputArg);
    WS_Connection_t server = pack->server;
    WS_Connection_t client = pack->client;
    switch (setjmp(jmp))
    {
    case SIGABRT:
        log_error("线程异常，已放弃");
        return NULL;
        break;
    case SIGSEGV:
        log_error("线程段错误，尝试恢复，本恢复可能造成部分资源无法完全释放");
        return NULL;
        break;
    default:
        break;
    }
    // 直接进入接收循环
    int rsnum; // 收发的数据量
    if (0 != pipe(fd))
    {
        log_error("创建管道失败");
        goto CLOSE;
    }
    int flag = fcntl(fd[1], F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(fd[1], F_SETFL, flag);
    flag = fcntl(fd[0], F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(fd[0], F_SETFL, flag);
    while (1)
    {
        rsnum = splice(server.sock, NULL, fd[1], NULL, 65535, SPLICE_F_MOVE | SPLICE_F_MORE | SPLICE_F_NONBLOCK);
        if (rsnum <= 0)
        {
            close(fd[0]);
            close(fd[1]);
            goto CLOSE;
        }
        rsnum = splice(fd[0], NULL, client.sock, NULL, 65535, SPLICE_F_MORE | SPLICE_F_MOVE);
        if (rsnum <= 0)
        {
            close(fd[0]);
            close(fd[1]);
            goto CLOSE;
        }
    }
CLOSE:
    shutdown(client.sock, SHUT_RDWR);
    shutdown(server.sock, SHUT_RDWR);
    return NULL;
}
