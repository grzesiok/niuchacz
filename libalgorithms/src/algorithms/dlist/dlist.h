#ifndef _LIBALGORITHMS_ALGORITHMS_DLIST_H
#define _LIBALGORITHMS_ALGORITHMS_DLIST_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/* 
 * Opaque pointer definitions. 
 * Implementation details and structural fields are strictly hidden inside dlist.c.
 */
typedef struct dlist_t dlist_t;

/* Publicly exported query structure representing flattened telemetry layout snapshots */
typedef struct dlist_query_t {
    uint64_t key;
    size_t size;
    uint32_t references;
    bool is_deleted;
    void *p_user_data;
} dlist_query_t;

/* Public API Management Lifecycles */
dlist_t* dlist_alloc(void);
void dlist_free(dlist_t *list);
void dlist_free_deleted_entries(dlist_t *list);

void* dlist_add(dlist_t *list, uint64_t key, const void *ptr, size_t size);
void dlist_del(dlist_t *list, void *ptr);
void dlist_release(void *ptr);

void* dlist_find(const dlist_t *list, uint64_t key);
void* dlist_get_first(const dlist_t *list);
void* dlist_get_last(const dlist_t *list);
bool dlist_is_empty(const dlist_t *list);

/* Querying / Iteration subsystem mechanisms */
bool dlist_query(const dlist_t *list, dlist_query_t *query, size_t *psize_inout);
dlist_query_t* dlist_query_next(dlist_query_t *query);
bool dlist_query_is_end(const dlist_query_t *query);

#endif /* _LIBALGORITHMS_ALGORITHMS_DLIST_H */