#ifndef _QUEUE_MANAGER_H
#define _QUEUE_MANAGER_H
#include "../svc_kernel/svc_kernel.h"
#include <sys/time.h>

//KSTATUS queuemgr_start(void);
//void queuemgr_stop(void);

#define QUEUE_MSG_FLAGS_CLEAR 0
#define QUEUE_MSG_FLAGS_ISLOCKED 1
#define QUEUE_MSG_FLAGS_ISDIRTY 2

typedef struct _QUEUE_MSG
{
	unsigned char _flags;
	size_t _msgsize;
	size_t _memsize;
	struct timeval _timestamp;
} QUEUE_MSG, *PQUEUE_MSG;

typedef struct _QUEUE
{
    void* _head;
    void* _tail;
    size_t _maxsize;
    size_t _leftsize;
    void* _leftborder;
    void* _rightborder;
} QUEUE, *PQUEUE;

KSTATUS queuemgr_create(PQUEUE *pqueue, size_t maxsize);
void queuemgr_destroy(PQUEUE pqueue);
KSTATUS queuemgr_enqueue(PQUEUE pqueue, struct timeval timestamp, void* ptr, size_t size);
KSTATUS queuemgr_dequeue(PQUEUE pqueue, struct timeval *timestamp, void* ptr, size_t *psize);
#endif
