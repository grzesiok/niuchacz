#include "queuemgr.h"

static void i_queue_memcpy(PQUEUE pqueue, void* dst, void* src, unsigned int size)
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

static void* i_queue_ptrmove(PQUEUE pqueue, void* dst, unsigned int size)
{
	unsigned long long ptrpos = (unsigned long long)dst+size;
	if(ptrpos >= (unsigned long long)pqueue->_rightborder)
	{
		ptrpos = (unsigned long long)pqueue->_leftborder+ptrpos-(unsigned long long)pqueue->_rightborder;
	}
	return (void*)ptrpos;
}

#define PTRMOVE(ptr, diff) (void*)((unsigned long long)ptr+diff)
#define MEM_ALIGN(val, alignment) ((val%alignment == 0) ? val : (val-(val%alignment)+alignment))

KSTATUS queuemgr_create(PQUEUE *pqueue, unsigned int maxsize)
{
	DPRINTF("queuemgr_create(maxsize=%llu)\n", maxsize);
	unsigned long long maxsize_aligned = MEM_ALIGN(maxsize, sizeof(QUEUE_MSG));
	PQUEUE pqueue_tmp = MALLOC2(QUEUE, 1, maxsize_aligned);
	if(pqueue_tmp == NULL)
		return KSTATUS_OUT_OF_MEMORY;
	memset(pqueue_tmp, 0, sizeof(QUEUE)+maxsize_aligned);
	pqueue_tmp->_leftborder = PTRMOVE(pqueue_tmp, sizeof(QUEUE));
	pqueue_tmp->_rightborder = PTRMOVE(pqueue_tmp->_leftborder, maxsize_aligned);
	pqueue_tmp->_head = pqueue_tmp->_leftborder;
	pqueue_tmp->_tail = pqueue_tmp->_leftborder;
	pqueue_tmp->_maxsize = maxsize_aligned;
	pqueue_tmp->_leftsize = pqueue_tmp->_maxsize;
	*pqueue = pqueue_tmp;
	DPRINTF("queuemgr_create: pqueue=%p lb=%p rb=%p h=%p l=%p alignment=%llu DONE\n", pqueue_tmp, pqueue_tmp->_leftborder, pqueue_tmp->_rightborder, pqueue_tmp->_head, pqueue_tmp->_tail, sizeof(QUEUE_MSG));
	return KSTATUS_SUCCESS;
}

void queuemgr_destroy(PQUEUE pqueue)
{
	DPRINTF("queuemgr_destroy(pqueue=%p)\n", pqueue);
	FREE(pqueue);
}

KSTATUS queuemgr_enqueue(PQUEUE pqueue, struct timeval timestamp, void* ptr, unsigned int size)
{
	DPRINTF("queuemgr_enqueue(pqueue=%p, ptr=%p, size=%llu)\n", pqueue, ptr, size);
	int memsize = MEM_ALIGN(size, sizeof(QUEUE_MSG))+sizeof(QUEUE_MSG);
	volatile PQUEUE_MSG pmsg;
__try_again:
	pmsg = (PQUEUE_MSG)pqueue->_head;
	unsigned char expected_flags = QUEUE_MSG_FLAGS_CLEAR;
	unsigned char desired_flags = QUEUE_MSG_FLAGS_ISLOCKED;
	if(!__atomic_compare_exchange(&pmsg->_flags, &expected_flags, &desired_flags, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE))
	{
		__builtin_ia32_pause();
		goto __try_again;
	}
	if(pqueue->_leftsize < memsize)
	{
		__atomic_store(&pmsg->_flags, &expected_flags, __ATOMIC_RELEASE);
		__builtin_ia32_pause();
		goto __try_again;
	}
	pmsg->_msgsize = size;
	pmsg->_memsize = memsize;
	pmsg->_timestamp = timestamp;
	pqueue->_leftsize -= pmsg->_memsize;
	PQUEUE_MSG pnew_head = (PQUEUE_MSG)i_queue_ptrmove(pqueue, pmsg, pmsg->_memsize);
	pnew_head->_flags = QUEUE_MSG_FLAGS_CLEAR;
	__atomic_store(&pqueue->_head, &pnew_head, __ATOMIC_RELEASE);
	i_queue_memcpy(pqueue, i_queue_ptrmove(pqueue, pmsg, sizeof(QUEUE_MSG)), ptr, pmsg->_msgsize);
	expected_flags = QUEUE_MSG_FLAGS_ISDIRTY;
	__atomic_store(&pmsg->_flags, &expected_flags, __ATOMIC_RELEASE);
	return KSTATUS_SUCCESS;
}

KSTATUS queuemgr_dequeue(PQUEUE pqueue, struct timeval *timestamp, void* ptr, unsigned int *psize)
{
	DPRINTF("queuemgr_dequeue(pqueue=%p)\n", pqueue);
	volatile PQUEUE_MSG pmsg;
__try_again:
	pmsg = (PQUEUE_MSG)pqueue->_tail;
	unsigned char expected_flags = QUEUE_MSG_FLAGS_ISDIRTY;
	unsigned char desired_flags = QUEUE_MSG_FLAGS_ISDIRTY|QUEUE_MSG_FLAGS_ISLOCKED;
	//waiting for dirty message which are not locked by write-process
	while(!__atomic_compare_exchange(&pmsg->_flags, &expected_flags, &desired_flags, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE))
	{
		__builtin_ia32_pause();
		goto __try_again;
	}
	//currently message is locked before any changes (read/write)
	//we need to move tail to another message
	void* new_tail = i_queue_ptrmove(pqueue, pmsg, pmsg->_memsize);
	__atomic_store(&pqueue->_tail, &new_tail, __ATOMIC_RELEASE);
	//copy it's data into some buffer
	i_queue_memcpy(pqueue, ptr, i_queue_ptrmove(pqueue, pmsg, sizeof(QUEUE_MSG)), pmsg->_msgsize);
	*psize = pmsg->_msgsize;
	*timestamp = pmsg->_timestamp;
	//and free memory
	desired_flags = QUEUE_MSG_FLAGS_CLEAR;
	__atomic_store(&pmsg->_flags, &desired_flags, __ATOMIC_RELEASE);
	pqueue->_leftsize += pmsg->_memsize;
	return KSTATUS_SUCCESS;
}
