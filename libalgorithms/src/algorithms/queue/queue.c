/*
 * src/algorithms/queue/queue.c
 * High-performance Process-Shared Ring Buffer Implementation (ANSI C/POSIX)
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>

#include "queue.h"

/* Concrete implementation of the opaque structure definition */
struct queue_t
{
    char *_head;
    char *_tail;
    void *_leftborder;
    void *_rightborder;
    pthread_mutex_t _readMutex;
    pthread_cond_t _readCondVariable;
    pthread_mutex_t _writeMutex;
    pthread_cond_t _writeCondVariable;
    int _consumers;
    int _producers;
    bool _isActive;

    volatile size_t _stats_EntriesCurrent;
    volatile size_t _stats_EntriesMax;
    volatile size_t _stats_MemUsageCurrent;
    volatile size_t _stats_MemUsageMax;
    volatile size_t _stats_MemSizeCurrent;
    volatile size_t _stats_MemSizeMin;
    volatile size_t _stats_MemSizeMax;
};

/* Calculation helper macro to allocate control layout alongside data buffer bounds */
#define QUEUE_TOTAL_ALLOC_SIZE(size) (sizeof(struct queue_t) + (size))

queue_t *queue_create(size_t size)
{
    pthread_mutexattr_t mattr;
    pthread_condattr_t cattr;

    /*
     * Allocate space for BOTH the queue_t control block AND the data arena
     * in a single shared, anonymous memory mapping. This ensures that when we fork,
     * all subprocesses write and read from the exact same physical memory block.
     */
    size_t total_bytes = QUEUE_TOTAL_ALLOC_SIZE(size);
    void *shm_block = mmap(NULL, total_bytes, PROT_READ | PROT_WRITE,
                           MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (shm_block == MAP_FAILED)
    {
        return NULL;
    }

    queue_t *pqueue = (queue_t *)shm_block;
    memset(pqueue, 0, sizeof(struct queue_t));

    /*
     * Set borders. Because MAP_SHARED guarantees identical virtual-to-physical
     * mapping characteristics directly inherited by fork() variants on modern Linux,
     * these absolute pointers remain valid and synchronized across forks.
     */
    pqueue->_leftborder = (char *)shm_block + sizeof(struct queue_t);
    pqueue->_rightborder = (char *)pqueue->_leftborder + size;
    pqueue->_head = (char *)pqueue->_leftborder;
    pqueue->_tail = (char *)pqueue->_leftborder;

    /* Initialize Mutexes with Cross-Process Capabilities */
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED); /* CRITICAL FOR FORK */
    pthread_mutex_init(&pqueue->_readMutex, &mattr);
    pthread_mutex_init(&pqueue->_writeMutex, &mattr);
    pthread_mutexattr_destroy(&mattr); /* Clean standard POSIX function name */

    /* Initialize Condition Variables with Cross-Process Capabilities */
    pthread_condattr_init(&cattr);
    pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED); /* CRITICAL FOR FORK */
    pthread_cond_init(&pqueue->_readCondVariable, &cattr);
    pthread_cond_init(&pqueue->_writeCondVariable, &cattr);
    pthread_condattr_destroy(&cattr);

    /* Set up baseline statistics and active tracking status flags */
    pqueue->_isActive = true;
    pqueue->_stats_MemSizeCurrent = size;
    pqueue->_stats_MemSizeMin = size;
    pqueue->_stats_MemSizeMax = size;

    return pqueue;
}

void queue_destroy(queue_t *pqueue)
{
    if (!pqueue)
        return;

    /* Mark inactive and wake up anyone trapped inside blocking calls */
    pthread_mutex_lock(&pqueue->_readMutex);
    pthread_mutex_lock(&pqueue->_writeMutex);
    pqueue->_isActive = false;
    pthread_cond_broadcast(&pqueue->_readCondVariable);
    pthread_cond_broadcast(&pqueue->_writeCondVariable);
    pthread_mutex_unlock(&pqueue->_writeMutex);
    pthread_mutex_unlock(&pqueue->_readMutex);

    /* Clean up the underlying primitives */
    pthread_cond_destroy(&pqueue->_readCondVariable);
    pthread_cond_destroy(&pqueue->_writeCondVariable);
    pthread_mutex_destroy(&pqueue->_readMutex);
    pthread_mutex_destroy(&pqueue->_writeMutex);

    /* Unmap the shared memory page block */
    size_t total_bytes = QUEUE_TOTAL_ALLOC_SIZE(pqueue->_stats_MemSizeCurrent);
    munmap((void *)pqueue, total_bytes);
}

bool queue_consumer_new(queue_t *pqueue)
{
    if (!pqueue)
        return false;
    pthread_mutex_lock(&pqueue->_readMutex);
    pqueue->_consumers++;
    pthread_mutex_unlock(&pqueue->_readMutex);
    return true;
}

void queue_consumer_free(queue_t *pqueue)
{
    if (!pqueue)
        return;
    pthread_mutex_lock(&pqueue->_readMutex);
    if (pqueue->_consumers > 0)
        pqueue->_consumers--;
    pthread_mutex_unlock(&pqueue->_readMutex);
}

bool queue_producer_new(queue_t *pqueue)
{
    if (!pqueue)
        return false;
    pthread_mutex_lock(&pqueue->_writeMutex);
    pqueue->_producers++;
    pthread_mutex_unlock(&pqueue->_writeMutex);
    return true;
}

void queue_producer_free(queue_t *pqueue)
{
    if (!pqueue)
        return;
    pthread_mutex_lock(&pqueue->_writeMutex);
    if (pqueue->_producers > 0)
        pqueue->_producers--;
    pthread_mutex_unlock(&pqueue->_writeMutex);
}

int queue_write(queue_t *pqueue, const void *pbuf, size_t nBytes, const struct timespec *timeout)
{
    if (!pqueue || !pbuf || nBytes == 0)
        return QUEUE_RET_ERROR;

    /*
     * FIX: Immediately reject payloads that exceed the total usable buffer size.
     * We subtract 1 because the ring buffer keeps 1 byte empty to tell full/empty apart.
     */
    size_t total_capacity = (char *)pqueue->_rightborder - (char *)pqueue->_leftborder;
    if (nBytes > (total_capacity - 1))
    {
        return QUEUE_RET_ERROR;
    }

    pthread_mutex_lock(&pqueue->_writeMutex);

    while (1)
    {
        if (!pqueue->_isActive)
        {
            pthread_mutex_unlock(&pqueue->_writeMutex);
            return QUEUE_RET_DESTROYING;
        }

        /* Compute current available capacity considering the ring wrap-around */
        size_t used_space = 0;
        if (pqueue->_tail >= pqueue->_head)
        {
            used_space = pqueue->_tail - pqueue->_head;
        }
        else
        {
            used_space = ((char *)pqueue->_rightborder - pqueue->_head) + ((char *)pqueue->_tail - (char *)pqueue->_leftborder);
        }

        size_t free_space = total_capacity - used_space - 1; /* Keep 1 byte boundary buffer */

        if (free_space >= nBytes)
        {
            /* Space available: Execute writing operation */
            size_t bytes_to_right = (char *)pqueue->_rightborder - pqueue->_tail;

            if (nBytes <= bytes_to_right)
            {
                memcpy(pqueue->_tail, pbuf, nBytes);
                pqueue->_tail += nBytes;
                if (pqueue->_tail == pqueue->_rightborder)
                {
                    pqueue->_tail = (char *)pqueue->_leftborder;
                }
            }
            else
            {
                memcpy(pqueue->_tail, pbuf, bytes_to_right);
                memcpy(pqueue->_leftborder, (char *)pbuf + bytes_to_right, nBytes - bytes_to_right);
                pqueue->_tail = (char *)pqueue->_leftborder + (nBytes - bytes_to_right);
            }

            /* Update tracking statistics variables */
            pqueue->_stats_EntriesCurrent++;
            if (pqueue->_stats_EntriesCurrent > pqueue->_stats_EntriesMax)
            {
                pqueue->_stats_EntriesMax = pqueue->_stats_EntriesCurrent;
            }
            pqueue->_stats_MemUsageCurrent += nBytes;
            if (pqueue->_stats_MemUsageCurrent > pqueue->_stats_MemUsageMax)
            {
                pqueue->_stats_MemUsageMax = pqueue->_stats_MemUsageCurrent;
            }

            /* Signal any consumers waiting on data inputs */
            pthread_cond_signal(&pqueue->_readCondVariable);
            pthread_mutex_unlock(&pqueue->_writeMutex);
            return (int)nBytes;
        }

        /* Queue is Full: Wait or handle timeout states */
        if (timeout)
        {
            int ret = pthread_cond_timedwait(&pqueue->_writeCondVariable, &pqueue->_writeMutex, timeout);
            if (ret == ETIMEDOUT)
            {
                pthread_mutex_unlock(&pqueue->_writeMutex);
                return QUEUE_RET_TIMEOUT;
            }
        }
        else
        {
            pthread_cond_wait(&pqueue->_writeCondVariable, &pqueue->_writeMutex);
        }
    }
}

int queue_read(queue_t *pqueue, void *pbuf, const struct timespec *timeout)
{
    if (!pqueue || !pbuf)
        return QUEUE_RET_ERROR;

    pthread_mutex_lock(&pqueue->_readMutex);

    while (1)
    {
        if (!pqueue->_isActive)
        {
            pthread_mutex_unlock(&pqueue->_readMutex);
            return QUEUE_RET_DESTROYING;
        }

        if (pqueue->_head != pqueue->_tail)
        {
            /* Queue contains data: Parse the next object out */
            size_t bytes_available = 0;
            if (pqueue->_tail > pqueue->_head)
            {
                bytes_available = pqueue->_tail - pqueue->_head;
            }
            else
            {
                bytes_available = (char *)pqueue->_rightborder - pqueue->_head;
            }

            memcpy(pbuf, pqueue->_head, bytes_available);
            pqueue->_head += bytes_available;
            if (pqueue->_head == pqueue->_rightborder)
            {
                pqueue->_head = (char *)pqueue->_leftborder;
            }

            /* Adjust stats counters */
            if (pqueue->_stats_EntriesCurrent > 0)
                pqueue->_stats_EntriesCurrent--;
            if (pqueue->_stats_MemUsageCurrent >= bytes_available)
            {
                pqueue->_stats_MemUsageCurrent -= bytes_available;
            }
            else
            {
                pqueue->_stats_MemUsageCurrent = 0;
            }

            /* Wake up any waiting producers stalling on full buffers */
            pthread_cond_signal(&pqueue->_writeCondVariable);
            pthread_mutex_unlock(&pqueue->_readMutex);
            return (int)bytes_available;
        }

        /* Queue is Empty: Wait or handle timeout states */
        if (timeout)
        {
            int ret = pthread_cond_timedwait(&pqueue->_readCondVariable, &pqueue->_readMutex, timeout);
            if (ret == ETIMEDOUT)
            {
                pthread_mutex_unlock(&pqueue->_readMutex);
                return QUEUE_RET_TIMEOUT;
            }
        }
        else
        {
            pthread_cond_wait(&pqueue->_readCondVariable, &pqueue->_readMutex);
        }
    }
}