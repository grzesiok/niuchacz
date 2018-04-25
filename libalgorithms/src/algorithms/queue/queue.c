#include "queue.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
	size_t _size;
} queue_entry_t;

static void* i_queue_memcpy_to(queue_t *pqueue, void* dst, const void* src, size_t size) {
	unsigned char* pdst = (unsigned char*)dst;
	unsigned char* psrc = (unsigned char*)src;
	while(size-- > 0) {
		*pdst++ = *psrc++;
		if(pdst == pqueue->_rightborder)
			pdst = pqueue->_leftborder;
	}
	return pdst;
}

static void* i_queue_memcpy_from(queue_t *pqueue, void* dst, const void* src, size_t size) {
	unsigned char* pdst = (unsigned char*)dst;
	unsigned char* psrc = (unsigned char*)src;
	while(size-- > 0) {
		*pdst++ = *psrc++;
		if(psrc == pqueue->_rightborder)
			psrc = pqueue->_leftborder;
	}
	return psrc;
}

queue_t* queue_create(size_t size) {
	queue_t *pqueue_tmp = malloc(size+sizeof(queue_t));
	if(pqueue_tmp == NULL)
		return NULL;
	pqueue_tmp->_leftborder = memoryPtrMove(pqueue_tmp, sizeof(queue_t));
	pqueue_tmp->_rightborder = memoryPtrMove(pqueue_tmp->_leftborder, size);
	pqueue_tmp->_head = pqueue_tmp->_leftborder;
	pqueue_tmp->_tail = pqueue_tmp->_leftborder;
	pqueue_tmp->_maxsize = size;
	pqueue_tmp->_leftsize = size;
	pthread_mutex_init(&pqueue_tmp->_readMutex, NULL);
	pthread_cond_init(&pqueue_tmp->_readCondVariable, NULL);
	pthread_mutex_init(&pqueue_tmp->_writeMutex, NULL);
	pthread_cond_init(&pqueue_tmp->_writeCondVariable, NULL);
	memset(pqueue_tmp->_leftborder, 0, size);
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
	while(pqueue->_leftsize == pqueue->_maxsize)
		pthread_cond_wait(&pqueue->_readCondVariable, &pqueue->_readMutex);
	// copy header as first bytes)
	pqueue->_tail = i_queue_memcpy_from(pqueue, &header, pqueue->_tail, sizeof(queue_entry_t));
	// then copy data
	pqueue->_tail = i_queue_memcpy_from(pqueue, pbuf, pqueue->_tail, header._size);
	__atomic_add_fetch(&pqueue->_leftsize, header._size+sizeof(queue_entry_t), __ATOMIC_RELEASE);
	// and broadcast changes to other threads
	pthread_cond_broadcast(&pqueue->_writeCondVariable);
	pthread_mutex_unlock(&pqueue->_readMutex);
	return header._size;
}

int queue_write(queue_t *pqueue, const void *pbuf, size_t nBytes, const struct timespec *timeout) {
	queue_entry_t header;

	if(nBytes+sizeof(queue_entry_t) >= pqueue->_maxsize)
		return -1;
	// prepare header
	header._size = nBytes;
	pthread_mutex_lock(&pqueue->_writeMutex);
	// check if we have enough room to store data
	while(pqueue->_leftsize <= header._size)
		pthread_cond_wait(&pqueue->_writeCondVariable, &pqueue->_writeMutex);
	// copy header
	pqueue->_head = i_queue_memcpy_to(pqueue, pqueue->_head, &header, sizeof(queue_entry_t));
	// and data
	pqueue->_head = i_queue_memcpy_to(pqueue, pqueue->_head, pbuf, header._size);
	__atomic_sub_fetch(&pqueue->_leftsize, header._size+sizeof(queue_entry_t), __ATOMIC_RELEASE);
	pthread_cond_broadcast(&pqueue->_readCondVariable);
	pthread_mutex_unlock(&pqueue->_writeMutex);
	return header._size;
}
