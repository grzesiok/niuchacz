#include "queuemgr.h"

static void i_queue_memcpy(PQUEUE pqueue, void* dst, void* src, unsigned long long size)
{
	DPRINTF("i_queue_memcpy(pqueue=%p, dst=%p, src=%p, size=%llu)\n", pqueue, dst, src, size);
	unsigned char* pdst = (unsigned char*)dst;
	unsigned char* psrc = (unsigned char*)src;
	while(size-- > 0)
	{
		if(pdst == pqueue->_rightborder)
			pdst = pqueue->_leftborder;
		*pdst++ = *psrc++;
	}
	DPRINTF("i_queue_memcpy: DONE\n");
}

static void* i_queue_ptrmove(PQUEUE pqueue, void* dst, unsigned long long size)
{
	DPRINTF("i_queue_ptrmove(pqueue=%p[lb=%p, rb=%p], dst=%p, size=%llu)\n", pqueue, pqueue->_leftborder, pqueue->_rightborder, dst, size);
	unsigned long long ptrpos = (unsigned long long)dst+size;
	if(ptrpos > (unsigned long long)pqueue->_rightborder)
	{
		ptrpos = (unsigned long long)pqueue->_leftborder+ptrpos-(unsigned long long)pqueue->_rightborder;
	}
	DPRINTF("i_queue_ptrmove: ptrpos=%p DONE\n", (void*)ptrpos);
	return (void*)ptrpos;
}
/*
static void* i_queue_ptrmove_for_headandtail(PQUEUE pqueue, void* dst, unsigned long long size)
{
	DPRINTF("i_queue_ptrmove(pqueue=%p[lb=%p, rb=%p], dst=%p, size=%llu)\n", pqueue, pqueue->_leftborder, pqueue->_rightborder, dst, size);
	unsigned long long ptrpos = (unsigned long long)dst+size;
	if(ptrpos > (unsigned long long)pqueue->_rightborder)
	{
		ptrpos = (unsigned long long)pqueue->_leftborder+ptrpos-(unsigned long long)pqueue->_rightborder;
	}
	//unsigned long long freeheadsize = (unsigned long long)pqueue->_rightborder-(unsigned long long)dst;
	//if(freeheadsize > size && freeheadsize < size+sizeof(QUEUE_MSG))
	//{
	//	DPRINTF("i_queue_ptrmove: freeheadsize=%llu size=$llu CHAINING ROW\n", freeheadsize, size);
	//	ptrpos = (unsigned long long)pqueue->_leftborder;
	//}
	DPRINTF("i_queue_ptrmove: ptrpos=%p DONE\n", (void*)ptrpos);
	return (void*)ptrpos;
}
*/
#define PTRMOVE(ptr, diff) (void*)((unsigned long)ptr+diff)
#define MEM_ALIGN(val, alignment) ((val%alignment == 0) ? val : (val-(val%alignment)+alignment))

KSTATUS queuemgr_create(PQUEUE *pqueue, unsigned long long maxsize)
{
	DPRINTF("queuemgr_create(maxsize=%llu)\n", maxsize);
	unsigned long long maxsize_aligned = MEM_ALIGN(maxsize, sizeof(QUEUE_MSG));
	PQUEUE pqueue_tmp = MALLOC2(QUEUE, 1, maxsize_aligned-sizeof(QUEUE));
	if(pqueue_tmp == NULL)
		return KSTATUS_OUT_OF_MEMORY;
	memset(pqueue_tmp, 0, maxsize_aligned);
	pqueue_tmp->_leftborder = PTRMOVE(pqueue_tmp, sizeof(QUEUE));
	pqueue_tmp->_rightborder = PTRMOVE(pqueue_tmp, maxsize_aligned);
	pqueue_tmp->_head = pqueue_tmp->_leftborder;
	pqueue_tmp->_tail = pqueue_tmp->_leftborder;
	pqueue_tmp->_maxsize = maxsize_aligned;
	pqueue_tmp->_leftsize = pqueue_tmp->_maxsize-sizeof(QUEUE);
	*pqueue = pqueue_tmp;
	DPRINTF("queuemgr_create: pqueue=%p lb=%p rb=%p h=%p l=%p alignment=%llu DONE\n", pqueue_tmp, pqueue_tmp->_leftborder, pqueue_tmp->_rightborder, pqueue_tmp->_head, pqueue_tmp->_tail, sizeof(QUEUE_MSG));
	return KSTATUS_SUCCESS;
}

void queuemgr_destroy(PQUEUE pqueue)
{
	DPRINTF("queuemgr_destroy(pqueue=%p)\n", pqueue);
	FREE(pqueue);
}

void dump_message(PQUEUE pqueue, PQUEUE_MSG pmsg)
{
	unsigned char* ptr = (unsigned char*)i_queue_ptrmove(pqueue, pmsg, sizeof(QUEUE_MSG));
	DPRINTF("msg(%p)[f=%llu ms=%llu, mems=%llu]\n", pmsg, pmsg->_flags, pmsg->_msgsize, pmsg->_memsize);
	DPRINTF("dump_msg(%p)[", pmsg);
	unsigned long long size = pmsg->_memsize;
	while(size-- > 0)
	{
		if(ptr == pqueue->_rightborder)
			ptr = pqueue->_leftborder;
		DPRINTF("%02x", *ptr++);
	}
	DPRINTF("]\n");
}

void list_messages(PQUEUE pqueue)
{
	DPRINTF("list_messages(%p)\n", pqueue);
	PQUEUE_MSG pmsg = (PQUEUE_MSG)pqueue->_tail;
	while(pmsg != pqueue->_head)
	{
		dump_message(pqueue, pmsg);
		pmsg = (PQUEUE_MSG)i_queue_ptrmove(pqueue, pmsg, pmsg->_memsize);
	}
	DPRINTF("list_messages(%p) DONE <===============\n", pqueue);
}

KSTATUS queuemgr_enqueue(PQUEUE pqueue, void* ptr, unsigned long long size)
{
	DPRINTF("queuemgr_enqueue(pqueue=%p, ptr=%p, size=%llu)\n", pqueue, ptr, size);
	int iter = 0;
	int memsize = MEM_ALIGN(size+sizeof(QUEUE_MSG), sizeof(QUEUE_MSG));
	volatile PQUEUE_MSG pmsg;
	DPRINTF("queuemgr_enqueue: head=%p size=%llu memsize=%llu\n", pqueue->_head, size, memsize);
__try_again:
	pmsg = (PQUEUE_MSG)pqueue->_head;
	unsigned char expected_flags = QUEUE_MSG_FLAGS_CLEAR;
	unsigned char desired_flags = QUEUE_MSG_FLAGS_ISLOCKED;
	if(!__atomic_compare_exchange(&pmsg->_flags, &expected_flags, &desired_flags, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE))
	{
		if(iter++ > 10)
		{
			DPRINTF("queuemgr_enqueue: TIMEOUT LOCK\n");
			return KSTATUS_UNSUCCESS;
		}
		__builtin_ia32_pause();
		goto __try_again;
	}
	DPRINTF("queuemgr_enqueue: LOCKED(ls=%llu ns=%llu, mems=%llu)\n", pqueue->_leftsize, size, memsize);
	if(pqueue->_leftsize < memsize)
	{
		__atomic_store(&pmsg->_flags, &expected_flags, __ATOMIC_RELEASE);
		if(iter++ > 10)
		{
			DPRINTF("queuemgr_enqueue: TIMEOUT SIZE\n");
			return KSTATUS_UNSUCCESS;
		}
		__builtin_ia32_pause();
		goto __try_again;
	}
	pmsg->_msgsize = size;
	pmsg->_memsize = memsize;
	pqueue->_leftsize -= pmsg->_memsize;
	PQUEUE_MSG pnew_head = (PQUEUE_MSG)i_queue_ptrmove(pqueue, pmsg, pmsg->_memsize);
	DPRINTF("queuemgr_enqueue: LOCKED+ALLOCATED nh=%p ls=%llu\n", pnew_head, pqueue->_leftsize);
	pnew_head->_flags = QUEUE_MSG_FLAGS_CLEAR;
	__atomic_store(&pqueue->_head, &pnew_head, __ATOMIC_RELEASE);
	i_queue_memcpy(pqueue, i_queue_ptrmove(pqueue, pmsg, sizeof(QUEUE_MSG)), ptr, pmsg->_msgsize);
	DPRINTF("queuemgr_enqueue: LOCKED+ALLOCATED+FILLED ms=%llu mems=%llu\n", pmsg->_msgsize, pmsg->_memsize);
	expected_flags = QUEUE_MSG_FLAGS_ISDIRTY;
	__atomic_store(&pmsg->_flags, &expected_flags, __ATOMIC_RELEASE);
	dump_message(pqueue, pmsg);
	return KSTATUS_SUCCESS;
}

KSTATUS queuemgr_dequeue(PQUEUE pqueue, void* ptr, unsigned long long *psize)
{
	DPRINTF("queuemgr_dequeue(pqueue=%p)\n", pqueue);
	list_messages(pqueue);
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
	pqueue->_leftsize += pmsg->_memsize;
	dump_message(pqueue, pmsg);
	//currently message is locked before any changes (read/write)
	//we need to move tail to another message
	void* new_tail = i_queue_ptrmove(pqueue, pmsg, pmsg->_memsize);
	DPRINTF("queuemgr_dequeue: FETCHED ot=%p nt=%p\n", pmsg, new_tail);
	__atomic_store(&pqueue->_tail, &new_tail, __ATOMIC_RELEASE);
	//copy it's data into some buffer
	i_queue_memcpy(pqueue, ptr, i_queue_ptrmove(pqueue, pmsg, sizeof(QUEUE_MSG)), pmsg->_msgsize);
	*psize = pmsg->_msgsize;
	//and free memory
	desired_flags = QUEUE_MSG_FLAGS_CLEAR;
	__atomic_store(&pmsg->_flags, &desired_flags, __ATOMIC_RELEASE);
	return KSTATUS_SUCCESS;
}
