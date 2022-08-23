#include <sys/socket.h>
#include <stdlib.h>
#include <setjmp.h>
#include <pthread.h>
#include "log.h"
#include "RemoteClient.h"

extern pthread_key_t Thread_Key;

SendingPack_t InitSending(int target_sock, int maxdata);
void *SendingThread(void *pack);
DataLink_t *UpSendingData(SendingPack_t *pack, DataLink_t *link, void *data, size_t size);
SendingPack_t InitSending(int target_sock, int maxdata)
{
    SendingPack_t pack = {0};
    pack.datanum = 0;
    pack.close = 0;
    pack.pool = ML_CreateMemPool(1024 * 1024);
    pthread_spin_init(&(pack.spinlock), PTHREAD_PROCESS_SHARED);
    pack.head = (DataLink_t *)ML_Malloc(&pack.pool, sizeof(DataLink_t));
    memset((pack.head),0,sizeof(DataLink_t));
    memset(&(pack.head->lock), 0, sizeof(pthread_mutex_t));
    pthread_mutex_lock(&(pack.head->lock));
    pack.maxdatanum = maxdata;
    pack.target_sock = target_sock;
    return pack;
}
DataLink_t *UpSendingData(SendingPack_t *pack, DataLink_t *link, void *data, size_t size)
{
    pthread_spin_lock(&(pack->spinlock));
    if (pack->close == 1)
    {
        pthread_mutex_unlock(&link->lock);
        pthread_spin_unlock(&(pack->spinlock));
        return NULL;
    }
    pthread_spin_unlock(&(pack->spinlock));
    while (pack->datanum > pack->maxdatanum)
    {
        usleep(10000);
    }
    link->data = data;
    link->datasize = size;
    link->next = (DataLink_t *)ML_Malloc(&pack->pool, sizeof(DataLink_t));
    memset(&(link->next->lock), 0, sizeof(pthread_mutex_t));
    link->next->data = NULL;
    pthread_mutex_lock(&(link->next->lock));
    pthread_spin_lock(&(pack->spinlock));
    pack->datanum += 1;
    pthread_spin_unlock(&(pack->spinlock));
    DataLink_t *backup = link->next;
    pthread_mutex_unlock(&(link->lock));
    return backup;
}

void *SendingThread(void *input)
{
    //设置线程独享资源
    jmp_buf jmp;
    pthread_setspecific(Thread_Key, &jmp);
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

    SendingPack_t *pack = input;
    DataLink_t *temp = pack->head;
    DataLink_t *freebackup;
    while (1)
    {
        pthread_mutex_lock(&(temp->lock));
        if (temp->data == NULL)
        {
            pthread_mutex_unlock(&(temp->lock));
            shutdown(pack->target_sock, SHUT_RDWR);
            ML_Free(&pack->pool, temp);
            pthread_exit(NULL);
        }
        if (0 >= write(pack->target_sock, temp->data, temp->datasize))
        {
            pthread_spin_lock(&(pack->spinlock));
            pack->close = 1;
            pthread_spin_unlock(&(pack->spinlock));
        }
        pthread_spin_lock(&(pack->spinlock));
        pack->datanum--;
        pthread_spin_unlock(&(pack->spinlock));
        ML_Free(&pack->pool, temp->data);
        freebackup = temp->next;
        pthread_mutex_unlock(&(temp->lock));
        ML_Free(&pack->pool, temp);
        temp = freebackup;
    }
}
