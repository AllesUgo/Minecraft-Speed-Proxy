#include <sys/socket.h>
#include <stdlib.h>
#include <pthread.h>
#include "RemoteClient.h"
SendingPack_t InitSending(int target_sock, int maxdata)
{
    SendingPack_t pack = {0};
    pack.target_sock = target_sock;
    pack.maxdata = maxdata;
    pack.mempool = ML_CreateMemPool(1024 * 1024);
    return pack;
}
void UpSendingData(SendingPack_t *pack, void *data, size_t datasize)
{
    pthread_mutex_lock(&(pack->lock));
    if (pack->datanum == -1)
    {
        pthread_mutex_unlock(&(pack->lock));
        ML_Free(&(pack->mempool), data);
        return;
    }
    if (pack->end == NULL)
    {
        fflush(stdout);
        pack->head = (DataLink_t *)ML_Malloc(&(pack->mempool), sizeof(DataLink_t));
        pack->head->data = data;
        pack->head->datasize = datasize;
        pack->head->next = NULL;
        pack->head->back = NULL;
        pack->end = pack->head;
    }
    else
    {
        //检查是否超过最大可用量
        while (pack->datanum == pack->maxdata)
        {
            pthread_cond_wait(&(pack->read), &(pack->lock));
            if (pack->datanum == -1)
            {
                pthread_mutex_unlock(&(pack->lock));
                ML_Free(&(pack->mempool), data);
                return;
            }
        }
        fflush(stdout);
        DataLink_t *temp = (DataLink_t *)ML_Malloc(&(pack->mempool), sizeof(DataLink_t));
        temp->data = data;
        temp->datasize = datasize;
        temp->back = pack->end;
        temp->next = NULL;
        if (pack->head != NULL)
        {
            pack->end->next = temp;
            pack->end = temp;
        }
        else
        {
            pack->head=temp;
            pack->end=temp;
        }
    }
    pack->datanum += 1;
    pthread_cond_signal(&(pack->write));
    pthread_mutex_unlock(&(pack->lock));
}
void MoveOutData(SendingPack_t *pack,struct DATAPACK*datapack)
{
    //struct DATAPACK datapack = {0};
    datapack->data=NULL;
    datapack->datasize=0;
    pthread_mutex_lock(&(pack->lock));
    while (pack->head == NULL)
    {
        if (pack->exit == 1)
        {
            pthread_mutex_unlock(&(pack->lock));
            return;
        }
        pthread_cond_wait(&(pack->write), &(pack->lock));
        if (pack->exit == 1)
        {
            pthread_mutex_unlock(&(pack->lock));
            return;
        }
    }

    datapack->data = pack->head->data;
    datapack->datasize = pack->head->datasize;
    DataLink_t *temp = pack->head;
    pack->head = pack->head->next;
    if (pack->head != NULL)
    {
        pack->head->back = NULL;
    }
    else
    {
        pack->end = NULL;
    }
    ML_Free(&(pack->mempool), temp);
    pack->datanum -= 1;
    pthread_cond_signal(&(pack->read));
    pthread_mutex_unlock(&(pack->lock));

    return;
}

void *SendingThread(void*input)
{
    SendingPack_t *pack=input;
    struct DATAPACK datapack;
    while (1)
    {
        MoveOutData(pack,&datapack);
        if (datapack.datasize == 0)
        {
            break;
        }
        if (0 >= write(pack->target_sock, datapack.data, datapack.datasize))
        {
            ML_Free(&(pack->mempool), datapack.data);
            shutdown(pack->target_sock, SHUT_RDWR);
            break;
        }

        ML_Free(&(pack->mempool), datapack.data);
    }
    pthread_mutex_lock(&(pack->lock));

    DataLink_t *temp1 = pack->head, *temp2;
    while (temp1 != NULL)
    {
        temp2 = temp1->next;
        ML_Free(&(pack->mempool), temp1->data);
        ML_Free(&(pack->mempool), temp1);
        temp1 = temp2;
        pack->datanum -= 1;
    }
    pack->head = NULL + 1;
    pack->end = NULL + 1;
    pack->datanum = -1;
    pthread_mutex_unlock(&(pack->lock));
    pthread_exit(NULL);
}
