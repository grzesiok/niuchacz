#ifndef _LIBALGORITHMS_ALGORITHMS_QUEUE_H
#define _LIBALGORITHMS_ALGORITHMS_QUEUE_H

typedef struct {
	unsigned char *_pbuf;
	int _head;
	int _tail;
	int _size;
} queue_t;

//This initializes the QUEUE structure with the given size
queue_t* queue_create(int size);
//This destroys buffer allocated for QUEUE
void queue_destroy(queue_t *pqueue);
//This reads nbytes bytes from the QUEUE
//The number of bytes read is returned
int queue_read(queue_t *pqueue, void *pbuf, int nBytes);
//This writes up to nbytes bytes to the QUEUE
//If the head runs in to the tail, not all bytes are written
//The number of bytes written is returned
int queue_write(queue_t *pqueue, const void *pbuf, int nBytes);

#endif /*_LIBALGORITHMS_ALGORITHMS_QUEUE_H */
