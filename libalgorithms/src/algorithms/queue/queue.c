#include "queue.h"
#include <stddef.h>
#include <stdlib.h>
#include "../../../include/memory.h"

queue_t* queue_init(int size) {
	queue_t* pqueue = (queue_t*)malloc(sizeof(queue_t)+size);
	if(pqueue == NULL)
		return NULL;
	pqueue->_head = 0;
	pqueue->_tail = 0;
	pqueue->_size = size;
	pqueue->_pbuf = (unsigned char*)memoryPtrMove(pqueue, sizeof(queue_t));
	return pqueue;
}

void queue_destroy(queue_t* pqueue) {
	free(pqueue);
}

int queue_read(queue_t *pqueue, void * buf, int nBytes) {
	int i;
	unsigned char * p;
	p = buf;
	for(i = 0; i < nBytes; i++) {
		if(pqueue->_tail != pqueue->_head) {
			*p++ = pqueue->_pbuf[pqueue->_tail];
			pqueue->_tail++;
			if(pqueue->_tail == pqueue->_size) {
				pqueue->_tail = 0;
			}
		} else {
			return i;
		}
	}
	return nBytes;
}

int queue_write(queue_t *pqueue, const void *pbuf, int nBytes) {
	int i;
	const unsigned char *p = pbuf;
	for(i = 0; i < nBytes; i++) {
		if((pqueue->_head + 1 == pqueue->_tail) || ((pqueue->_head + 1 == pqueue->_size) && (pqueue->_tail == 0))) {
				return i;
		} else {
			pqueue->_pbuf[pqueue->_head] = *p++;
			pqueue->_head++;
			if(pqueue->_head == pqueue->_size) {
				pqueue->_head = 0;
			}
		}
	}
	return nBytes;
}
