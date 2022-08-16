#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include "log.h"
#include "websocket.h"
#include "RemoteClient.h"

extern char *remoteServerAddress;
extern int LocalPort;
extern int Remote_Port;

void *SendingThread(SendingPack_t *pack);
SendingPack_t InitSending(int target_sock, int maxdata);
void UpSendingData(SendingPack_t *pack, void *data, size_t datasize);

void *DealRemote(void *InputArg);
void *DealRemote(void *InputArg)
{
    //接收多线程传参
    Rcpack *pack = ((Rcpack *)InputArg);
    WS_Connection_t server = pack->server;
    WS_Connection_t client = pack->client;
    char data[8192];

    //直接进入接收循环
    register int rsnum; //收发的数据量
    //数据包
    SendingPack_t spack = InitSending(client.sock, 20);
    pthread_t pid;
    pthread_create(&pid, NULL, SendingThread, &spack);
    void *stackdata;

    while (1)
    {
        stackdata = ML_Malloc(&(spack.mempool), 128);
        rsnum = read(server.sock, stackdata, 128);
        if (rsnum <= 0)
        {
            ML_Free(&(spack.mempool), stackdata);
            shutdown(client.sock,SHUT_RDWR);
            shutdown(server.sock,SHUT_RDWR);
            break;
        }
        // printf("S R%d\n",rsnum);

        UpSendingData(&spack, stackdata, rsnum);
    }
    pthread_mutex_lock(&(spack.lock));
    spack.exit = 1;
    pthread_cond_signal(&(spack.write));
    pthread_mutex_unlock(&(spack.lock));
    pthread_join(pid, NULL);
    pthread_cond_destroy(&(spack.read));
    pthread_cond_destroy(&(spack.write));
    pthread_mutex_destroy(&(spack.lock));
    if (ML_DestoryMem(&spack.mempool) != ML_SUCCESS)
    {
        printf("内存未完全释放\n");
    }
    return NULL;
}
