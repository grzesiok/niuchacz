#include "queue.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <immintrin.h>

typedef struct {
    size_t _size;
    uint32_t _crc32;
} queue_entry_t;

uint32_t sse42_crc32(const uint8_t *bytes, size_t len) {
    uint32_t hash = 0xffffffff;
    size_t i = 0;
    for(i = 0; i < len; i++) {
        hash = _mm_crc32_u8(hash, bytes[i]);
    }
    return hash;
}

static void i_queue_write(queue_t *pqueue, const void* src, size_t size) {
    unsigned char* psrc = (unsigned char*)src;
    while(size-- > 0) {
        *pqueue->_head++ = *psrc++;
        if(pqueue->_head == pqueue->_rightborder)
            pqueue->_head = pqueue->_leftborder;
    }
}

static void i_queue_read(queue_t *pqueue, void* dst, size_t size) {
    unsigned char* pdst = (unsigned char*)dst;
    while(size-- > 0) {
        *pdst++ = *pqueue->_tail++;
        if(pqueue->_tail == pqueue->_rightborder)
            pqueue->_tail = pqueue->_leftborder;
    }
}

queue_t* queue_create(size_t size) {
    bool all_mutexes_properly_initialized = true;
    queue_t *pqueue_tmp = malloc(size+sizeof(queue_t));
    if(pqueue_tmp == NULL)
        return NULL;
    pqueue_tmp->_leftborder = memoryPtrMove(pqueue_tmp, sizeof(queue_t));
    pqueue_tmp->_rightborder = memoryPtrMove(pqueue_tmp->_leftborder, size);
    pqueue_tmp->_head = pqueue_tmp->_leftborder;
    pqueue_tmp->_tail = pqueue_tmp->_leftborder;
    pqueue_tmp->_consumers = 0;
    pqueue_tmp->_producers = 0;
    pqueue_tmp->_isActive = true;
    pqueue_tmp->_stats_EntriesCurrent = 0;
    pqueue_tmp->_stats_EntriesMax = 0;
    pqueue_tmp->_stats_MemUsageCurrent = 0;
    pqueue_tmp->_stats_MemUsageMax = 0;
    pqueue_tmp->_stats_MemSizeCurrent = size;
    pqueue_tmp->_stats_MemSizeMin = size;
    pqueue_tmp->_stats_MemSizeMax = size;
    if(pthread_mutex_init(&pqueue_tmp->_readMutex, NULL) != 0) {
        all_mutexes_properly_initialized = false;
    }
    if(pthread_cond_init(&pqueue_tmp->_readCondVariable, NULL) != 0) {
        all_mutexes_properly_initialized = false;
    }
    if(pthread_mutex_init(&pqueue_tmp->_writeMutex, NULL) != 0) {
        all_mutexes_properly_initialized = false;
    }
    if(pthread_cond_init(&pqueue_tmp->_writeCondVariable, NULL) != 0) {
        all_mutexes_properly_initialized = false;
    }
    if(!all_mutexes_properly_initialized) {
        queue_destroy(pqueue_tmp);
        return NULL;
    }
    memset(pqueue_tmp->_leftborder, 0, size);
    return pqueue_tmp;
}

void queue_destroy(queue_t* pqueue) {
    if(pqueue == NULL)
        return;
    pqueue->_isActive = false;
    pthread_mutex_lock(&pqueue->_writeMutex);
    while(pqueue->_producers > 0) {
        pthread_cond_wait(&pqueue->_writeCondVariable, &pqueue->_writeMutex);
    }
    pthread_mutex_unlock(&pqueue->_writeMutex);
    pthread_mutex_lock(&pqueue->_readMutex);
    while(pqueue->_consumers > 0) {
        pthread_cond_wait(&pqueue->_readCondVariable, &pqueue->_readMutex);
    }
    pthread_mutex_unlock(&pqueue->_readMutex);
    pthread_mutex_destroy(&pqueue->_readMutex);
    pthread_cond_destroy(&pqueue->_readCondVariable);
    pthread_mutex_destroy(&pqueue->_writeMutex);
    pthread_cond_destroy(&pqueue->_writeCondVariable);
    free(pqueue);
}

bool queue_consumer_new(queue_t* pqueue) {
    bool success;
    pthread_mutex_lock(&pqueue->_readMutex);
    if(pqueue->_isActive) {
        pqueue->_consumers++;
        success = true;
    } else success = false;
    pthread_mutex_unlock(&pqueue->_readMutex);
    return success;
}

void queue_consumer_free(queue_t* pqueue) {
    pthread_mutex_lock(&pqueue->_readMutex);
    pqueue->_consumers--;
    pthread_mutex_unlock(&pqueue->_readMutex);
    pthread_cond_broadcast(&pqueue->_readCondVariable);
}

bool queue_producer_new(queue_t* pqueue) {
    bool success;
    pthread_mutex_lock(&pqueue->_writeMutex);
    if(pqueue->_isActive) {
        pqueue->_producers++;
        success = true;
    } else success = false;
    pthread_mutex_unlock(&pqueue->_writeMutex);
    return success;
}

void queue_producer_free(queue_t* pqueue) {
    pthread_mutex_lock(&pqueue->_writeMutex);
    pqueue->_producers--;
    pthread_mutex_unlock(&pqueue->_writeMutex);
    pthread_cond_broadcast(&pqueue->_writeCondVariable);
}

int queue_read(queue_t *pqueue, void *pbuf, const struct timespec *timeout) {
    queue_entry_t header;
    int ret;

    pthread_mutex_lock(&pqueue->_readMutex);
    // if tail and head are the same => no entries in queue waiting for read
    while(pqueue->_stats_MemUsageCurrent == 0) {
        if(timeout != NULL) {
            ret = pthread_cond_timedwait(&pqueue->_readCondVariable, &pqueue->_readMutex, timeout);
        } else ret = pthread_cond_wait(&pqueue->_readCondVariable, &pqueue->_readMutex);
        if(!pqueue->_isActive) {
            pthread_mutex_unlock(&pqueue->_readMutex);
            return QUEUE_RET_DESTROYING;
        }
        if(ret == ETIMEDOUT) {
            pthread_mutex_unlock(&pqueue->_readMutex);
            return QUEUE_RET_TIMEOUT;
        }
    }
    // copy header as first bytes)
    i_queue_read(pqueue, &header, sizeof(queue_entry_t));
    // then copy data
    i_queue_read(pqueue, pbuf, header._size);
    __atomic_sub_fetch(&pqueue->_stats_EntriesCurrent, 1, __ATOMIC_RELEASE);
    __atomic_sub_fetch(&pqueue->_stats_MemUsageCurrent, header._size+sizeof(queue_entry_t), __ATOMIC_RELEASE);
    pthread_mutex_unlock(&pqueue->_readMutex);
    // and broadcast changes to other threads
    pthread_cond_broadcast(&pqueue->_writeCondVariable);
    if(sse42_crc32(pbuf, header._size) != header._crc32) {
        return QUEUE_RET_ERROR;
    }
    return header._size;
}

int queue_write(queue_t *pqueue, const void *pbuf, size_t nBytes, const struct timespec *timeout) {
    queue_entry_t header;
    size_t entrySize = nBytes+sizeof(queue_entry_t);
    int ret;

    if(entrySize >= pqueue->_stats_MemSizeCurrent)
        return QUEUE_RET_ERROR;
    // prepare header
    header._size = nBytes;
    header._crc32 = sse42_crc32(pbuf, nBytes);
    pthread_mutex_lock(&pqueue->_writeMutex);
    // check if we have enough room to store data
    while(pqueue->_stats_MemSizeCurrent-pqueue->_stats_MemUsageCurrent < entrySize) {
        if(timeout != NULL) {
            ret = pthread_cond_timedwait(&pqueue->_writeCondVariable, &pqueue->_writeMutex, timeout);
        } else ret = pthread_cond_wait(&pqueue->_writeCondVariable, &pqueue->_writeMutex);
        if(!pqueue->_isActive) {
            pthread_mutex_unlock(&pqueue->_writeMutex);
            return QUEUE_RET_DESTROYING;
        }
        if(ret == ETIMEDOUT) {
            pthread_mutex_unlock(&pqueue->_writeMutex);
            return QUEUE_RET_TIMEOUT;
        }
    }
    // copy header
    i_queue_write(pqueue, &header, sizeof(queue_entry_t));
    // and data
    i_queue_write(pqueue, pbuf, header._size);
    size_t numOfEntries = __atomic_add_fetch(&pqueue->_stats_EntriesCurrent, 1, __ATOMIC_RELEASE);
    if(numOfEntries > pqueue->_stats_EntriesMax)
        pqueue->_stats_EntriesMax = numOfEntries;
    size_t usageOfMemory = __atomic_add_fetch(&pqueue->_stats_MemUsageCurrent, entrySize, __ATOMIC_RELEASE);
    if(usageOfMemory > pqueue->_stats_MemUsageMax)
        pqueue->_stats_MemUsageMax = usageOfMemory;
    pthread_mutex_unlock(&pqueue->_writeMutex);
    pthread_cond_broadcast(&pqueue->_readCondVariable);
    return header._size;
}
