#include "queue.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
	size_t _size;
} queue_entry_t;

static void i_queue_memcpy(queue_t *pqueue, void* dst, const void* src, size_t size)
{
	unsigned char* pdst = (unsigned char*)dst;
	unsigned char* psrc = (unsigned char*)src;
	while(size-- > 0)
	{
		if(pdst == pqueue->_rightborder)
			pdst = pqueue->_leftborder;
		*pdst++ = *psrc++;
	}
}

static void* i_queue_ptrmove(queue_t *pqueue, void* dst, size_t size)
{
	unsigned long long ptrpos = (unsigned long long)dst+size;
	if(ptrpos >= (unsigned long long)pqueue->_rightborder)
	{
		ptrpos = (unsigned long long)pqueue->_leftborder+ptrpos-(unsigned long long)pqueue->_rightborder;
	}
	return (void*)ptrpos;
}

queue_t* queue_create(size_t size) {
	size_t maxsize_aligned = memoryAlign(size, sizeof(queue_t));
	queue_t *pqueue_tmp = malloc(sizeof(queue_t)+maxsize_aligned);
	if(pqueue_tmp == NULL)
		return NULL;
	pqueue_tmp->_leftborder = memoryPtrMove(pqueue_tmp, sizeof(queue_t));
	pqueue_tmp->_rightborder = memoryPtrMove(pqueue_tmp->_leftborder, maxsize_aligned);
	pqueue_tmp->_head = pqueue_tmp->_leftborder;
	pqueue_tmp->_tail = pqueue_tmp->_leftborder;
	pqueue_tmp->_maxsize = maxsize_aligned;
	pthread_mutex_init(&pqueue_tmp->_readMutex, NULL);
	pthread_cond_init(&pqueue_tmp->_readCondVariable, NULL);
	pthread_mutex_init(&pqueue_tmp->_writeMutex, NULL);
	pthread_cond_init(&pqueue_tmp->_writeCondVariable, NULL);
	memset(pqueue_tmp->_leftborder, 0, maxsize_aligned);
	return pqueue_tmp;
}

void queue_destroy(queue_t* pqueue) {
	pthread_mutex_destroy(&pqueue->_readMutex);
	pthread_cond_destroy(&pqueue->_readCondVariable);
	pthread_mutex_destroy(&pqueue->_writeMutex);
	pthread_cond_destroy(&pqueue->_writeCondVariable);
	free(pqueue);
}

int queue_read(queue_t *pqueue, void *pbuf, const struct timespec *timeout) {
	queue_entry_t header;

	pthread_mutex_lock(&pqueue->_readMutex);
	// if tail and head are the same => no entries in queue waiting for read
	while(pqueue->_tail == pqueue->_head)
		pthread_cond_wait(&pqueue->_readCondVariable, &pqueue->_readMutex);
	// copy header as first bytes)
	i_queue_memcpy(pqueue, &header, pqueue->_tail, sizeof(queue_entry_t));
	// then copy data
	i_queue_memcpy(pqueue, pbuf, i_queue_ptrmove(pqueue, pqueue->_tail, sizeof(queue_entry_t)), header._size);
	// and finally increase tail pointer
	pqueue->_tail = i_queue_ptrmove(pqueue, pqueue->_tail, header._size+sizeof(queue_entry_t));
	// and broadcast changes to other threads
	pthread_cond_broadcast(&pqueue->_writeCondVariable);
	pthread_mutex_unlock(&pqueue->_readMutex);
	return header._size;
}

int queue_write(queue_t *pqueue, const void *pbuf, size_t nBytes, const struct timespec *timeout) {
	queue_entry_t header;

	// prepare header
	header._size = nBytes;
	pthread_mutex_lock(&pqueue->_writeMutex);
	// check if we have enough room to store data
	while(i_queue_ptrmove(pqueue, pqueue->_head, header._size+sizeof(queue_entry_t)) == pqueue->_tail)
		pthread_cond_wait(&pqueue->_writeCondVariable, &pqueue->_writeMutex);
	// copy header
	i_queue_memcpy(pqueue, pqueue->_head, &header, sizeof(queue_entry_t));
	// and data
	i_queue_memcpy(pqueue, i_queue_ptrmove(pqueue, pqueue->_head, sizeof(queue_entry_t)), pbuf, header._size);
	// and increase head
	pqueue->_head = i_queue_ptrmove(pqueue, pqueue->_head, header._size+sizeof(queue_entry_t));
	pthread_cond_broadcast(&pqueue->_readCondVariable);
	pthread_mutex_unlock(&pqueue->_writeMutex);
	return header._size;
}
