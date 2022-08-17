#ifndef RC
#define RC
#include"websocket.h"
#include"mempool.h"
typedef struct RCPACK
{
    WS_Connection_t client;
    WS_Connection_t server;
}Rcpack;
typedef struct DATALINK
{
    void *data;
    size_t datasize;
    pthread_mutex_t lock;
    struct DATALINK *next;
} DataLink_t;
typedef struct SENDINGPACK
{
    ML_Pool_t pool;
    char close;
    DataLink_t *head;
    pthread_spinlock_t spinlock;
    pid_t pid;
    int maxdatanum;
    int datanum;
    int target_sock;
} SendingPack_t;


struct DATAPACK
{
    void *data;
    size_t datasize;
};
#endif