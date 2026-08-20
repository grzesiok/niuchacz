#ifndef _LIBALGORITHMS_ALGORITHMS_QUEUE_H
#define _LIBALGORITHMS_ALGORITHMS_QUEUE_H

#include <stddef.h>
#include <stdbool.h>
#include <time.h>

/* 
 * Opaque pointer definition. 
 * The layout of struct queue_t is hidden from the outside world.
 */
typedef struct queue_t queue_t;

/* Status and error return codes */
#define QUEUE_RET_ERROR      -1
#define QUEUE_RET_TIMEOUT    -2
#define QUEUE_RET_DESTROYING -3

/* Public API Function Prototypes */
queue_t* queue_create(size_t size);
void queue_destroy(queue_t *pqueue);

bool queue_consumer_new(queue_t *pqueue);
void queue_consumer_free(queue_t *pqueue);
bool queue_producer_new(queue_t *pqueue);
void queue_producer_free(queue_t *pqueue);

/*
 * Read and write framed messages.
 * - `queue_write` writes a single logical entry prefixed by a 4-byte length header.
 * - `queue_read` allocates a buffer for the next complete entry payload and returns
 *   it via the `void **pbuf` output parameter; the caller is responsible for freeing it.
 *
 * Note: Function names are preserved for compatibility within this project, but
 * the semantics are framed (length-prefixed) entries rather than raw stream copies.
 */
int queue_read(queue_t *pqueue, void **pbuf, const struct timespec *timeout);
int queue_write(queue_t *pqueue, const void *pbuf, size_t nBytes, const struct timespec *timeout);

#endif /* _LIBALGORITHMS_ALGORITHMS_QUEUE_H */