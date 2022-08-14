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
    struct DATALINK *back;
    struct DATALINK *next;
} DataLink_t;
typedef struct SENDINGPACK
{
    char exit;
    DataLink_t *head;
    DataLink_t *end;
    pthread_mutex_t lock;
    pid_t pid;
    pthread_cond_t write;
    pthread_cond_t read;
    int maxdata;
    int datanum;
    int target_sock;
    ML_Pool_t mempool;
} SendingPack_t;


struct DATAPACK
{
    void *data;
    size_t datasize;
};
#endif