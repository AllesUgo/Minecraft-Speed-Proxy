#include "mempool.h"
#include <stdio.h>
#include <stdlib.h>
#define true 1
#define false 0
typedef struct ML_POOL_LINK
{
	char useing;
	void *ptr;
	size_t size;
	struct ML_POOL_LINK *next;
} Link_t;

ML_Pool_t ML_CreateMemPool(size_t max_freetemp)
{
	ML_Pool_t pool = {0};
	pool.head = NULL;
	pool.max_freetemp = max_freetemp;
	pool.canuse = true;
	return pool;
}
void *ML_Malloc(ML_Pool_t *pool, size_t size)
{
	#ifdef MEMPOOL_DISABLE
	return malloc(size);
	#endif
	pthread_mutex_lock(&(pool->lock));
	if (pool->canuse == false)
	{
		printf("内存池不可用\n");
		fflush(stdout);
		fflush(stderr);
		exit(1);
	}
	Link_t *head = (Link_t *)(pool->head);
	if (head == NULL)
	{
		head = (Link_t *)malloc(sizeof(Link_t));
		head->next = NULL;
		head->ptr = malloc(size);
		if (head->ptr == NULL)
		{
			free(head);
			pthread_mutex_unlock(&(pool->lock));
			return NULL;
		}
		else
		{
			head->size = size;
			head->useing = true;
			pool->head = head;
			pthread_mutex_unlock(&(pool->lock));
			return head->ptr;
		}
	}
	else
	{
		Link_t *temp = head, *appropriate = NULL;
		size_t appropriate_size = 0;
		//先找到第一个可用的内存单元
		while (temp != NULL)
		{
			if (temp->size >= size && temp->useing == false)
			{
				appropriate = temp;
				appropriate_size = temp->size;
			}
			temp = temp->next;
		}
		//检查是否有可用的块
		if (appropriate == NULL)
		{
			//没有可用的内存块，需要添加一个内存块
			//使用头插法
			temp = head;
			head = (Link_t *)malloc(sizeof(Link_t)); //申请节点内存
			head->ptr = malloc(size);
			if (head->ptr == NULL)
			{
				//内存申请失败
				free(head);
				pthread_mutex_unlock(&(pool->lock));
				return NULL;
			}
			head->size = size;
			head->useing = true;
			head->next = temp;
			pool->head = head;
			pthread_mutex_unlock(&(pool->lock));
			return head->ptr;
		}
		else
		{
			//有可用的内存块，继续寻找最合适的
			while (temp != NULL)
			{
				if (temp->size >= size && temp->useing == false && temp->size < appropriate_size)
				{
					appropriate_size = temp->size;
					appropriate = temp;
				}
				temp = temp->next;
			}
			//检索完成
			appropriate->useing = true;
			pthread_mutex_unlock(&(pool->lock));
			return appropriate->ptr;
		}
	}
}
void ML_Free(ML_Pool_t *pool, void *ptr)
{
	#ifdef MEMPOOL_DISABLE
	free(ptr);
	return;
	#endif
	pthread_mutex_lock(&(pool->lock));
	if (pool->canuse == false)
	{
		printf("内存池不可用\n");
		fflush(stdout);
		fflush(stderr);
		exit(1);
	}
	Link_t *head = (Link_t *)(pool->head);
	if (head == NULL || ptr == NULL)
	{
		fputs("试图从释放内存池中没有的内存\n", stderr);
		fflush(stdout);
		fflush(stderr);
		pthread_mutex_unlock(&(pool->lock));
		return;
	}
	size_t cache_size = 0;
	Link_t *temp = head;
	char success = false;
	while (temp != NULL)
	{
		if (temp->useing == false)
			cache_size = cache_size + temp->size;
		if (temp->ptr == ptr && temp->useing == true)
		{
			temp->useing = false;
			cache_size = cache_size + temp->size;
			success = true;
		}
		temp = temp->next;
	}
	if (success == false)
	{
		fputs("试图从释放内存池中没有的内存\n", stderr);
		fflush(stdout);
		fflush(stderr);
		pthread_mutex_unlock(&(pool->lock));
		return;
	}
	if (cache_size > pool->max_freetemp)
	{
		//超过最大缓存，清理缓存
		temp = head->next;
		Link_t *temp_before = head;
		while (temp != NULL)
		{
			if (temp->useing == false)
			{
				temp_before->next = temp->next;
				free(temp->ptr);
				free(temp);
				temp = temp_before->next;
				continue;
			}
			temp = temp->next;
			temp_before = temp_before->next;
		}
		//最后检查头节点
		if (head->useing == false)
		{
			temp = head;
			head = head->next;
			free(temp->ptr);
			free(temp);
			pool->head = head;
		}
	}
	pthread_mutex_unlock(&(pool->lock));
}
void *ML_CheekMemLeak(ML_Pool_t pool)
{
	#ifdef MEMPOOL_DISABLE
	return NULL;
	#endif
	pthread_mutex_lock(&pool.lock);
	Link_t *temp = (Link_t *)pool.head;
	while (temp != NULL)
	{
		if (temp->useing == true)
		{
			pthread_mutex_unlock(&pool.lock);
			return temp->ptr;
		}
		temp = temp->next;
	}
	pthread_mutex_unlock(&pool.lock);
	return NULL;
}

int ML_DestoryMem(ML_Pool_t *pool)
{
	#ifdef MEMPOOL_DISABLE
	return ML_SUCCESS;
	#endif
	pthread_mutex_lock(&(pool->lock));
	if (pool->canuse == false)
	{
		printf("ML_DestoryMem:内存池不可用\n");
		fflush(stdout);
		fflush(stderr);
		exit(1);
	}
	pthread_mutex_unlock(&(pool->lock));
	if (ML_CheekMemLeak(*pool) != NULL)
	{
		printf("ML_DestoryMem内存池非空\n");
		return ML_FAIL;
	}
	pthread_mutex_lock(&(pool->lock));
	Link_t *temp = (Link_t *)(pool->head), *temp_next;
	while (temp != NULL)
	{
		temp_next = temp->next;
		free(temp->ptr);
		free(temp);
		temp = temp_next;
	}
	pool->canuse = false;
	pool->head = NULL;
	pool->max_freetemp = 0;
	pthread_mutex_unlock(&(pool->lock));
	return ML_SUCCESS;
}
void ML_DestoryMemForced(ML_Pool_t *pool)
{
	#ifdef MEMPOOL_DISABLE
	return;
	#endif
	pthread_mutex_lock(&(pool->lock));
	if (pool->canuse == false)
	{
		printf("ML_DestoryMemForced:内存池不可用\n");
		fflush(stdout);
		fflush(stderr);
		exit(1);
	}
	Link_t *temp = (Link_t *)(pool->head), *temp_next;
	while (temp != NULL)
	{
		temp_next = temp->next;
		free(temp->ptr);
		free(temp);
		temp = temp_next;
	}
	pool->canuse = false;
	pool->head = NULL;
	pool->max_freetemp = 0;
	pthread_mutex_unlock(&(pool->lock));
}
void ML_FreeAllForced(ML_Pool_t *pool)
{
	#ifdef MEMPOOL_DISABLE
	return;
	#endif
	pthread_mutex_lock(&(pool->lock));
	if (pool->canuse == false)
	{
		fflush(stdout);
		fflush(stderr);
		exit(1);
	}
	Link_t *temp = (Link_t *)(pool->head), *temp_next;
	while (temp != NULL)
	{
		temp_next = temp->next;
		free(temp->ptr);
		free(temp);
		temp = temp_next;
	}
	pool->head = NULL;
	pthread_mutex_unlock(&(pool->lock));
}
int ML_DetachMem(ML_Pool_t *pool, void *ptr)
{
	#ifdef MEMPOOL_DISABLE
	return ML_SUCCESS;
	#endif
	pthread_mutex_lock(&(pool->lock));
	if (pool->canuse == false)
	{
		fflush(stdout);
		fflush(stderr);
		exit(1);
	}
	Link_t *head = (Link_t *)(pool->head);
	Link_t *temp = head->next;
	Link_t *temp_before = head;
	while (temp != NULL)
	{
		if (temp->useing == true && temp->ptr == ptr)
		{
			temp_before->next = temp->next;
			free(temp);
			temp = temp_before->next;
			pthread_mutex_unlock(&(pool->lock));
			return ML_SUCCESS;
		}
		temp = temp->next;
		temp_before = temp_before->next;
	}
	temp = head;
	if (head->next == ptr && head->useing == true)
	{
		temp = head;
		head = head->next;
		free(temp);
		pool->head = head;
		pthread_mutex_unlock(&(pool->lock));
		return ML_SUCCESS;
	}
	pthread_mutex_unlock(&(pool->lock));
	return ML_FAIL;
}
