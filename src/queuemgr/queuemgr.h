#ifndef _QUEUE_MANAGER_H
#define _QUEUE_MANAGER_H
#include "../svc_kernel/svc_kernel.h"

//KSTATUS queuemgr_start(void);
//void queuemgr_stop(void);

#define QUEUE_MSG_FLAGS_CLEAR 0
#define QUEUE_MSG_FLAGS_ISLOCKED 1
#define QUEUE_MSG_FLAGS_ISDIRTY 2

typedef struct _QUEUE_MSG
{
	unsigned char _flags;
	unsigned long long _msgsize;
	unsigned long long _memsize;
} QUEUE_MSG, *PQUEUE_MSG;

typedef struct _QUEUE
{
    void* _head;
    void* _tail;
    unsigned long long _maxsize;
    unsigned long long _leftsize;
    void* _leftborder;
    void* _rightborder;
} QUEUE, *PQUEUE;

KSTATUS queuemgr_create(PQUEUE *pqueue, unsigned long long maxsize);
void queuemgr_destroy(PQUEUE pqueue);
KSTATUS queuemgr_enqueue(PQUEUE pqueue, void* ptr, unsigned long long size);
KSTATUS queuemgr_dequeue(PQUEUE pqueue, void* ptr, unsigned long long *psize);
#endif
