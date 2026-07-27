#include "dlist.h"
#include "algorithms/spinlock/spinlock.h"
#include <string.h>

/* Hidden list node headers for internal link stitching calculations */
typedef struct dlist_node_t {
    struct dlist_node_t *next;
    struct dlist_node_t *prev;
} dlist_node_t;

/* Private data block wrappers wrapping around specific client allocations */
typedef struct {
    dlist_node_t header;
    uint64_t key;
    size_t size;
    uint32_t references;
    bool is_deleted;
} dlist_entry_t;

/* True architectural representation of the concealed list controller descriptor */
struct dlist_t {
    dlist_node_t active_entries;
    dlist_node_t deleted_entries;
    spinlock_t active_lock;
    spinlock_t deleted_lock;
};

// ====================================================================
// INTERNAL API: EMBEDDED NODE HEADER OPERATIONS
// ====================================================================

/* Initialize a node to point to itself, creating a circular link base */
static void i_dlist_node_init(dlist_node_t *node) {
    if (node == NULL)
        return;
    node->next = node;
    node->prev = node;
}

/* Retrieve the immediate next node pointer in the sequence */
static dlist_node_t* i_dlist_node_next(const dlist_node_t *node) {
    return node ? node->next : NULL;
}

/* Retrieve the immediate previous node pointer in the sequence */
static dlist_node_t* i_dlist_node_prev(const dlist_node_t *node) {
    return node ? node->prev : NULL;
}

/* Add a new node entry immediately after the specified anchor/list head position */
static void i_dlist_node_add(dlist_node_t *list, dlist_node_t *node) {
    if (list == NULL || node == NULL)
        return;
    node->next = list->next;
    node->prev = list;
    list->next->prev = node;
    list->next = node;
}

/* Unlink a node from its neighbors and reset it to a clean, isolated state */
static void i_dlist_node_del(dlist_node_t *node) {
    if (node == NULL || node->prev == NULL || node->next == NULL)
        return;
    node->prev->next = node->next;
    node->next->prev = node->prev;
    i_dlist_node_init(node);
}

/* Check if the circular list contains zero data entries (points only to itself) */
static bool i_dlist_node_is_empty(const dlist_node_t *list) {
    if (list == NULL)
        return true;
    return (list->next == list);
}

/* Determine if the cursor node has looped back to hit the base list boundary anchor */
static bool i_dlist_node_is_end(const dlist_node_t *list, const dlist_node_t *node) {
    return (node == list);
}

// ====================================================================
// INTERNAL API: DATA ENTRY OPERATIONS
// ====================================================================

/* Initialize the embedded node header within the entry structure */
static void i_dlist_entry_init(dlist_entry_t *entry) {
    if (entry == NULL)
        return;
    i_dlist_node_init(&entry->header);
}

/* Navigate to the next full data entry wrapper using header linking offsets */
static dlist_entry_t* i_dlist_entry_next(const dlist_entry_t *entry) {
    if (entry == NULL)
        return NULL;
    return (dlist_entry_t*)i_dlist_node_next(&entry->header);
}

/* Navigate to the previous full data entry wrapper using header linking offsets */
static dlist_entry_t* i_dlist_entry_prev(const dlist_entry_t *entry) {
    if (entry == NULL)
        return NULL;
    return (dlist_entry_t*)i_dlist_node_prev(&entry->header);
}

/* Attach a full data entry structure directly into the targeted node pipeline anchor */
static void i_dlist_entry_add(dlist_node_t *list, dlist_entry_t *entry) {
    if (list == NULL || entry == NULL)
        return;
    i_dlist_node_add(list, &entry->header);
}

/* Sever a data entry structure from its neighbors and reset its link layout */
static void i_dlist_entry_del(dlist_entry_t *entry) {
    if (entry == NULL)
        return;
    i_dlist_node_del(&entry->header);
}

/* Check if the tracking cursor entry points back to the base list sentinel boundary */
static bool i_dlist_entry_is_end(const dlist_node_t *list, const dlist_entry_t *entry) {
    if (list == NULL || entry == NULL)
        return true;
    return i_dlist_node_is_end(list, &entry->header);
}

/* 
 * Safe pointer arithmetic abstractions leveraging uintptr_t 
 * to bypass memory alignment and compilation architecture truncation warnings.
 */
#define move_ptr_to_user_data(ptr) ((void*)((uintptr_t)(ptr) + sizeof(dlist_entry_t)))
#define move_user_data_to_ptr(ptr) ((void*)((uintptr_t)(ptr) - sizeof(dlist_entry_t)))

// ====================================================================
// EXTERNAL API: PUBLIC LIFECYCLE MANAGEMENT ROUTINES
// ====================================================================

/* Allocate and initialize a new doubly linked list instance descriptor */
dlist_t* dlist_alloc(void) {
    dlist_t *list = (dlist_t*)malloc(sizeof(dlist_t));
    if (list == NULL)
        return NULL;

    i_dlist_node_init(&list->active_entries);
    i_dlist_node_init(&list->deleted_entries);

    /* Initialize custom spinlocks safely using defined architecture tokens */
    list->active_lock = SPINLOCK_UNLOCKED;
    list->deleted_lock = SPINLOCK_UNLOCKED;

    return list;
}

/* Deep free execution path to tear down all active and lazy-deleted list entries */
void dlist_free(dlist_t *list) {
    if (list == NULL)
        return;

    /* Acquire synchronization locks to ensure multi-threaded execution sanity */
    spinlockLock(&list->active_lock);
    spinlockLock(&list->deleted_lock);

    /* Safely release memory allocation blocks from the active pool pipeline */
    dlist_entry_t *entry = (dlist_entry_t*)i_dlist_node_next(&list->active_entries);
    while (!i_dlist_entry_is_end(&list->active_entries, entry)) {
        /* FIXED: Cache the next pointer BEFORE invoking free() to avoid Use-After-Free bugs */
        dlist_entry_t *next_entry = (dlist_entry_t*)i_dlist_node_next(&entry->header);
        i_dlist_entry_del(entry);
        free(entry);
        entry = next_entry;
    }

    /* Safely release memory allocation blocks from the lazy-deleted zombie pool pipeline */
    entry = (dlist_entry_t*)i_dlist_node_next(&list->deleted_entries);
    while (!i_dlist_entry_is_end(&list->deleted_entries, entry)) {
        /* FIXED: Cache the next pointer BEFORE invoking free() to avoid heap corruption */
        dlist_entry_t *next_entry = (dlist_entry_t*)i_dlist_node_next(&entry->header);
        i_dlist_entry_del(entry);
        free(entry);
        entry = next_entry;
    }

    /* Locks do not need to be un-set or unlocked since the core memory handle is recycled */
    free(list);
}

/* Asynchronous garbage collection sweep to purge records with zero remaining references */
void dlist_free_deleted_entries(dlist_t *list) {
    if (list == NULL)
        return;

    spinlockLock(&list->deleted_lock);
    dlist_entry_t *entry = (dlist_entry_t*)i_dlist_node_next(&list->deleted_entries);

    while (!i_dlist_entry_is_end(&list->deleted_entries, entry)) {
        /* Atomic load read ensures thread execution sync barriers across processor caches */
        if (entry->is_deleted && __atomic_load_n(&entry->references, __ATOMIC_ACQUIRE) == 0) {
            dlist_entry_t *entry_to_free = entry;
            entry = i_dlist_entry_next(entry);
            i_dlist_entry_del(entry_to_free);
            free(entry_to_free);
        } else {
            entry = i_dlist_entry_next(entry);
        }
    }
    spinlockUnlock(&list->deleted_lock);
}

// ====================================================================
// EXTERNAL API: CORE DATA MANIPULATION ROUTINES
// ====================================================================

/* Insert a new key-value payload segment entry into the active list pipeline */
void* dlist_add(dlist_t *list, uint64_t key, const void *ptr, size_t size) {
    if (list == NULL)
        return NULL;

    dlist_entry_t *entry = (dlist_entry_t*)malloc(sizeof(dlist_entry_t) + size);
    if (entry == NULL)
        return NULL;

    i_dlist_entry_init(entry);
    entry->references = 1;
    entry->is_deleted = false;
    entry->key = key;
    entry->size = size;

    if (ptr && size > 0) {
        memcpy(move_ptr_to_user_data(entry), ptr, size);
    }

    spinlockLock(&list->active_lock);
    i_dlist_entry_add(&list->active_entries, entry);
    spinlockUnlock(&list->active_lock);

    return move_ptr_to_user_data(entry);
}

/* Detach an entry from active tracking and shift it to the tombstone list if references exist */
void dlist_del(dlist_t *list, void *ptr) {
    if (list == NULL || ptr == NULL)
        return;

    dlist_entry_t *entry = move_user_data_to_ptr(ptr);

    /* Isolate and unlink from active pipeline nodes safely */
    spinlockLock(&list->active_lock);
    i_dlist_entry_del(entry);    
    spinlockUnlock(&list->active_lock); /* FIXED: Unlock immediately to bypass hierarchical nested deadlock configurations */

    /* If concurrent software threads retain active reference claims, keep structure alive inside tombstone pool */
    if (__atomic_load_n(&entry->references, __ATOMIC_ACQUIRE) > 0) {
        entry->is_deleted = true;
        spinlockLock(&list->deleted_lock);
        i_dlist_entry_add(&list->deleted_entries, entry);
        spinlockUnlock(&list->deleted_lock);
    } else {
        /* No components hold reference counters, immediate clean memory reclaim path is fully legal */
        free(entry);
    }
}

/* Query active list indices to locate entry mappings corresponding to target unique keys */
void* dlist_find(const dlist_t *list, uint64_t key) {
    if (list == NULL)
        return NULL;

    /* Const cast required due to internal design state requirements of spinlock mutation parameters */
    spinlock_t *lock = (spinlock_t*)&list->active_lock;
    spinlockLock(lock);
    
    dlist_entry_t *entry = (dlist_entry_t*)i_dlist_node_next(&list->active_entries);
    while (!i_dlist_entry_is_end(&list->active_entries, entry)) {
        if (entry->key == key) {
            __atomic_add_fetch(&entry->references, 1, __ATOMIC_RELEASE);
            spinlockUnlock(lock);
            return move_ptr_to_user_data(entry);
        }
        entry = i_dlist_entry_next(entry);
    }
    
    spinlockUnlock(lock);
    return NULL;
}

/* Safely pop and retrieve the oldest non-deleted item position header pointer */
void* dlist_get_first(const dlist_t *list) {
    if (list == NULL)
        return NULL;

    spinlock_t *lock = (spinlock_t*)&list->active_lock;
    spinlockLock(lock);
    
    dlist_entry_t *entry = (dlist_entry_t*)i_dlist_node_next(&list->active_entries);
    while (!i_dlist_entry_is_end(&list->active_entries, entry)) {
        if (!entry->is_deleted) {
            __atomic_add_fetch(&entry->references, 1, __ATOMIC_RELEASE);
            spinlockUnlock(lock);
            return move_ptr_to_user_data(entry);
        }
        entry = i_dlist_entry_next(entry);
    }
    
    /* FIXED: Prevented atomic fetch increments executing directly over uninitialized sentinel list head addresses */
    spinlockUnlock(lock);
    return NULL;
}

/* Safely pop and retrieve the newest non-deleted item position header pointer */
void* dlist_get_last(const dlist_t *list) {
    if (list == NULL)
        return NULL;

    spinlock_t *lock = (spinlock_t*)&list->active_lock;
    spinlockLock(lock);
    
    dlist_entry_t *entry = (dlist_entry_t*)i_dlist_node_prev(&list->active_entries);
    while (!i_dlist_entry_is_end(&list->active_entries, entry)) {
        if (!entry->is_deleted) {
            __atomic_add_fetch(&entry->references, 1, __ATOMIC_RELEASE);
            spinlockUnlock(lock);
            return move_ptr_to_user_data(entry);
        }
        entry = i_dlist_entry_prev(entry);
    }
    
    /* FIXED: Prevented atomic fetch increments executing directly over uninitialized sentinel list head addresses */
    spinlockUnlock(lock);
    return NULL;
}

/* Yield back active thread claim holds over targeted list element memory buffers */
void dlist_release(void *ptr) {
    if (ptr == NULL)
        return;
    dlist_entry_t *entry = move_user_data_to_ptr(ptr);
    __atomic_sub_fetch(&entry->references, 1, __ATOMIC_RELEASE);
}

/* Evaluate current state metrics to evaluate if any elements occupy structural active links */
bool dlist_is_empty(const dlist_t *list) {
    if (list == NULL)
        return true;
    return i_dlist_node_is_empty(&list->active_entries);
}

// ====================================================================
// EXTERNAL API: FILTERING QUERY MECHANISM SUBSYSTEM
// ====================================================================

#define move_ptr_to_query_user_data(ptr) ((void*)((uintptr_t)(ptr) + sizeof(dlist_query_t)))

/* Process sequential list node records and dump payload snapshots into a serialized search buffer array */
static bool i_dlist_query_single_list(const dlist_node_t *list_base, dlist_query_t **pquery, size_t *psize_inout, size_t *pcurrent_size) {
    size_t size_out = *pcurrent_size;
    dlist_query_t *query_element = *pquery;
    dlist_entry_t *entry = (dlist_entry_t*)i_dlist_node_next(list_base);
    
    while (!i_dlist_entry_is_end(list_base, entry)) {
        /* Verify available destination allocation spans can safely support the upcoming record plus trailing NULL */
        if (size_out + sizeof(dlist_query_t) + entry->size + sizeof(void*) > *psize_inout) {
            *pcurrent_size = size_out;
            *pquery = query_element;
            return false;
        }
        
        query_element->key = entry->key;
        query_element->references = __atomic_load_n(&entry->references, __ATOMIC_ACQUIRE);
        query_element->is_deleted = entry->is_deleted;
        query_element->size = entry->size;
        
        if (entry->size > 0) {
            memcpy(move_ptr_to_query_user_data(query_element), move_ptr_to_user_data(entry), entry->size);
        }
        query_element->p_user_data = move_ptr_to_query_user_data(query_element);
        
        size_out += sizeof(dlist_query_t) + entry->size;
        query_element = dlist_query_next(query_element);
        entry = i_dlist_entry_next(entry);
    }
    
    *pcurrent_size = size_out;
    *pquery = query_element;
    return true;
}

/* Flatten and extract structural snapshots covering active and lazy-deleted list states into linear array formats */
bool dlist_query(const dlist_t *list, dlist_query_t *query, size_t *psize_inout) {
    size_t size_out = 0;
    bool active_records_pulled = false;
    bool deleted_records_pulled = false;
    dlist_query_t *query_start_anchor = query;

    if (list == NULL || query == NULL || psize_inout == NULL) {
        if (psize_inout) *psize_inout = 0;
        return false;
    }
        
    spinlockLock((spinlock_t*)&list->active_lock);
    if (i_dlist_query_single_list(&list->active_entries, &query, psize_inout, &size_out)) {
        active_records_pulled = true;
    }
    spinlockUnlock((spinlock_t*)&list->active_lock);
    
    spinlockLock((spinlock_t*)&list->deleted_lock);
    if (i_dlist_query_single_list(&list->deleted_entries, &query, psize_inout, &size_out)) {
        deleted_records_pulled = true;
    }
    spinlockUnlock((spinlock_t*)&list->deleted_lock);

    /* FIXED: Write explicit binary termination tokens precisely at the final element offset boundaries */
    if (size_out + sizeof(void*) <= *psize_inout) {
        void **terminator = (void**)((uintptr_t)query_start_anchor + size_out);
        *terminator = NULL;
    }

    *psize_inout = size_out;
    return active_records_pulled && deleted_records_pulled;
}

/* Advance structural query pointer layouts using localized calculation shifts */
dlist_query_t* dlist_query_next(dlist_query_t *query) {
    if (query == NULL)
        return NULL;
    return (dlist_query_t*)((uintptr_t)query + sizeof(dlist_query_t) + query->size);
}

/* Inspect final data offsets to determine if iteration parameters hit terminal boundaries */
bool dlist_query_is_end(const dlist_query_t *query) {
    if (query == NULL)
        return true;
    /* Safely fetch dereferenced boundary representations to match tail markers */
    void *ptr = *((void**)query);
    return (ptr == NULL);
}