#ifndef _LIBALGORITHMS_ALGORITHMS_QUEUE_H
#define _LIBALGORITHMS_ALGORITHMS_QUEUE_H

#include <stddef.h>
#include <pthread.h>
#include "../../../include/memory.h"
#include <stdbool.h>

typedef struct {
    char* _head;
    char* _tail;
    void* _leftborder;
    void* _rightborder;
    pthread_mutex_t _readMutex;
    pthread_cond_t _readCondVariable;
    pthread_mutex_t _writeMutex;
    pthread_cond_t _writeCondVariable;
    int _consumers;
    int _producers;
    bool _isActive;

    volatile size_t _stats_EntriesCurrent;//number of entries actually stored in queue
    volatile size_t _stats_EntriesMax;//maximum number of entries stored in queue
    volatile size_t _stats_MemUsageCurrent;//Aktual usage of memory for stored entries
    volatile size_t _stats_MemUsageMax;//Maximum amount of memory needed for entries
    volatile size_t _stats_MemSizeCurrent;//Current size of queue
    volatile size_t _stats_MemSizeMin;//Minimum size of queue
    volatile size_t _stats_MemSizeMax;//Maximum size of queue
} queue_t;

#define QUEUE_RET_ERROR -1
#define QUEUE_RET_TIMEOUT -2
#define QUEUE_RET_DESTROYING -3

queue_t* queue_create(size_t size);
void queue_destroy(queue_t* pqueue);
bool queue_consumer_new(queue_t* pqueue);
void queue_consumer_free(queue_t* pqueue);
bool queue_producer_new(queue_t* pqueue);
void queue_producer_free(queue_t* pqueue);
int queue_read(queue_t *pqueue, void *pbuf, const struct timespec *timeout);
int queue_write(queue_t *pqueue, const void *pbuf, size_t nBytes, const struct timespec *timeout);

#endif /*_LIBALGORITHMS_ALGORITHMS_QUEUE_H */
