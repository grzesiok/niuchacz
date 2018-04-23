#ifndef _LIBALGORITHMS_ALGORITHMS_QUEUE_H
#define _LIBALGORITHMS_ALGORITHMS_QUEUE_H

#include <stddef.h>
#include <pthread.h>
#include "../../../include/memory.h"

typedef struct {
	void* _head;
	void* _tail;
    size_t _maxsize;
    void* _leftborder;
    void* _rightborder;
    pthread_mutex_t _readMutex;
    pthread_cond_t _readCondVariable;
    pthread_mutex_t _writeMutex;
    pthread_cond_t _writeCondVariable;
} queue_t;

queue_t* queue_create(size_t size);
void queue_destroy(queue_t* pqueue);
int queue_read(queue_t *pqueue, void *pbuf, const struct timespec *timeout);
int queue_write(queue_t *pqueue, const void *pbuf, size_t nBytes, const struct timespec *timeout);


#endif /*_LIBALGORITHMS_ALGORITHMS_QUEUE_H */
