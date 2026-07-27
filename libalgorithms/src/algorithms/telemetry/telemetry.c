#include "telemetry.h"
#include "ext/art/art.h"
#include "algorithms/dlist/dlist.h"
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdio.h>

struct telemetry_list_t {
    char *name;
    art_tree tree;
};

struct telemetry_entry_t {
    telemetry_behavior_t flags;
    uint64_t value; /* Handled via lock-free thread-safe atomic compiler built-ins */
};

typedef struct {
    dlist_t *manager_list;
} telemetry_mgr_t;

static telemetry_mgr_t g_telemetry_mgr = { .manager_list = NULL };

// ====================================================================
// INTERNAL API: ITERATION TREE CALLBACKS AND HELPERS
// ====================================================================

static int i_telemetry_dump_callback(void *data, const unsigned char *key, uint32_t key_len, void *value) {
    (void)data;
    telemetry_entry_t *entry = (telemetry_entry_t*)value;
    uint64_t current_val = __atomic_load_n(&entry->value, __ATOMIC_ACQUIRE);
    
    printf("[TELEMETRY_DUMP] Entry flags=%08x value=%20"PRIu64" name=%.*s\n", 
           entry->flags, current_val, (int)key_len, key);
    return 0;
}

static int i_telemetry_dump_list(const telemetry_list_t *list) {
    if (list == NULL) return -1;
    printf("[TELEMETRY_DUMP] List name=%s active_entries=%"PRIu64"\n", list->name, (uint64_t)art_size((art_tree*)&list->tree));
    if (art_iter((art_tree*)&list->tree, i_telemetry_dump_callback, NULL) != 0) {
        return -1;
    }
    return 0;
}

// ====================================================================
// EXTERNAL API: SERVICE SINGLETON LIFECYCLES
// ====================================================================

int telemetry_mgr_start(void) {
    g_telemetry_mgr.manager_list = (dlist_t*)dlist_alloc();
    return (g_telemetry_mgr.manager_list == NULL) ? -1 : 0;
}

void telemetry_mgr_stop(void) {
    if (g_telemetry_mgr.manager_list != NULL) {
        dlist_free_deleted_entries(g_telemetry_mgr.manager_list);
        dlist_free(g_telemetry_mgr.manager_list);
        g_telemetry_mgr.manager_list = NULL;
    }
}

int telemetry_mgr_dump(void) {
    char buff[4096];
    dlist_query_t *pquery = (dlist_query_t*)buff;
    size_t required_size = sizeof(buff);

    if (g_telemetry_mgr.manager_list == NULL) return -1;
    
    printf("[TELEMETRY] FRAMEWORK BULK DUMP START ...\n");
    
    if (!dlist_query(g_telemetry_mgr.manager_list, pquery, &required_size)) {
        printf("[TELEMETRY] Dump warning: target lookup query buffer overflow constraints!\n");
        return -1;
    }
    
    while (!dlist_query_is_end(pquery)) {
        i_telemetry_dump_list((telemetry_list_t*)pquery->p_user_data);
        pquery = dlist_query_next(pquery);
    }
    
    printf("[TELEMETRY] FRAMEWORK BULK DUMP STOP ...\n");
    return 0;
}

// ====================================================================
// EXTERNAL API: FACTORIES AND METRICS ALLOCATORS
// ====================================================================

telemetry_list_t* telemetry_list_create(const char *list_name) {
    if (list_name == NULL || g_telemetry_mgr.manager_list == NULL) return NULL;

    size_t name_len = strlen(list_name);
    telemetry_list_t *tmp_list = (telemetry_list_t*)malloc(sizeof(telemetry_list_t) + name_len + 1);
    if (tmp_list == NULL) return NULL;
        
    tmp_list->name = (char*)((uintptr_t)tmp_list + sizeof(telemetry_list_t));
    strcpy(tmp_list->name, list_name);
    
    if (art_tree_init(&tmp_list->tree) != 0) {
        free(tmp_list);
        return NULL;
    }
    
    uint64_t routing_key = 0;
    memcpy(&routing_key, list_name, (name_len > 8) ? 8 : name_len);

    telemetry_list_t *bound_list = (telemetry_list_t*)dlist_add(g_telemetry_mgr.manager_list, routing_key, tmp_list, sizeof(telemetry_list_t) + name_len + 1);
    bound_list->name = (char*)((uintptr_t)bound_list + sizeof(telemetry_list_t));
    
    free(tmp_list);
    return bound_list;
}

void telemetry_list_destroy(telemetry_list_t *list) {
    if (list == NULL || g_telemetry_mgr.manager_list == NULL) return;
    dlist_del(g_telemetry_mgr.manager_list, list);
    art_tree_destroy(&list->tree);
    dlist_release(list);
}

int telemetry_alloc(telemetry_list_t *list, const char *name, telemetry_behavior_t flags, telemetry_entry_t *entry) {
    if (list == NULL || name == NULL || entry == NULL) return -1;

    entry->flags = flags;
    entry->value = 0;
    
    if (art_insert(&list->tree, (const unsigned char*)name, strlen(name), entry) == entry) {
        return -1;
    }
    return 0;
}

int telemetry_alloc_bulk(telemetry_list_t *list, const telemetry_bulk_init_t *stats_array, int stats_num) {
    if (list == NULL || stats_array == NULL) return -1;

    for (int i = 0; i < stats_num; i++) {
        int status = telemetry_alloc(list, stats_array[i].key, stats_array[i].flags, stats_array[i].ptr);
        if (status != 0) {
            return status;
        }
    }
    return 0;
}

void telemetry_free(telemetry_list_t *list, const char *name) {
    if (list && name) art_delete(&list->tree, (const unsigned char*)name, strlen(name));
}

telemetry_entry_t* telemetry_find(const telemetry_list_t *list, const char *name) {
    if (!list || !name) return NULL;
    return (telemetry_entry_t*)art_search((art_tree*)&list->tree, (const unsigned char*)name, strlen(name));
}

uint64_t telemetry_update(telemetry_entry_t *entry, uint64_t value) {
    if (entry == NULL) return 0;

    switch (entry->flags) {
        case TELEMETRY_TYPE_SUM:
            return __atomic_add_fetch(&entry->value, value, __ATOMIC_RELEASE);
        case TELEMETRY_TYPE_LAST:
            __atomic_store_n(&entry->value, value, __ATOMIC_RELEASE);
            return value;
    }
    return 0;
}

uint64_t telemetry_get_value(const telemetry_entry_t *entry) {
    if (entry == NULL) return 0;
    return __atomic_load_n(&entry->value, __ATOMIC_ACQUIRE);
}