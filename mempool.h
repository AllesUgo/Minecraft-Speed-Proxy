#ifndef MEMPOOL
#define MEMPOOL
#include<malloc.h>
#include<pthread.h>
#define ML_FAIL -1
#define ML_SUCCESS 0


typedef struct MEMPOOLHEAD {
	char canuse;
	void* head;
	size_t max_freetemp;
	pthread_mutex_t lock;
} ML_Pool_t;

ML_Pool_t ML_CreateMemPool(size_t max_freetemp);
void* ML_Malloc(ML_Pool_t *pool, size_t size);
void ML_Free(ML_Pool_t *pool, void* ptr);
void* ML_CheekMemLeak(ML_Pool_t pool);//返回NULL为没有泄露，否则返回找到的第一块泄露的地址
int ML_DestoryMem(ML_Pool_t *pool);
void ML_DestoryMemForced(ML_Pool_t *pool);
void ML_FreeAllForced(ML_Pool_t* pool);
int ML_DetachMem(ML_Pool_t* pool, void* ptr);
#endif
