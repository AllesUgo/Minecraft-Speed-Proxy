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

void *SendingThread(void *pack);
SendingPack_t InitSending(int target_sock, int maxdata);
DataLink_t *UpSendingData(SendingPack_t *pack, DataLink_t *link, void *data, size_t size);

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
    SendingPack_t spack = InitSending(client.sock, 2000);
    pthread_t pid;
    pthread_create(&pid, NULL, SendingThread, &spack);
    void *stackdata;
    DataLink_t*temp=spack.head;
    while (1)
    {
        stackdata = ML_Malloc(&spack.pool,8192*4);
        rsnum = read(server.sock, stackdata, 8192*4);
        
        if (rsnum <= 0)
        {
            ML_Free(&spack.pool, stackdata);
            pthread_mutex_unlock(&(temp->lock));
            shutdown(client.sock, SHUT_RDWR);
            shutdown(server.sock, SHUT_RDWR);
            break;
        }
        temp = UpSendingData(&spack, temp, stackdata, rsnum);
        if (temp == NULL)
        {
            //另一侧连接已断开
            ML_Free(&spack.pool, stackdata);
            shutdown(client.sock, SHUT_RDWR);
            shutdown(server.sock, SHUT_RDWR);
            break;
        }
    }
    pthread_join(pid, NULL);
    pthread_spin_destroy(&(spack.spinlock));
    if (NULL!=ML_CheekMemLeak(spack.pool))
        {
            printf("内存泄露\n");
        }
    return NULL;
}
