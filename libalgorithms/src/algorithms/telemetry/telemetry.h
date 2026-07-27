#ifndef _LIBALGORITHMS_TELEMETRY_H
#define _LIBALGORITHMS_TELEMETRY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* 
 * Opaque pointer definitions for core telemetry handles.
 * Internal structure details are entirely hidden inside telemetry.c.
 */
typedef struct telemetry_list_t telemetry_list_t;
typedef struct telemetry_entry_t telemetry_entry_t;

/* Supported metrics data accumulation behaviors */
typedef enum {
    TELEMETRY_TYPE_SUM  = 1, /* Aggregates metrics values sequentially */
    TELEMETRY_TYPE_LAST = 2  /* Overwrites storage slots with the latest input value */
} telemetry_behavior_t;

/* Blueprint structure used for safe, non-repetitive telemetry bulk array initializations */
typedef struct {
    const char *key;
    telemetry_behavior_t flags;
    telemetry_entry_t *ptr;
} telemetry_bulk_init_t;

/* Master Service Telemetry Manager Lifecycles (Returns 0 on success, -1 on error) */
int telemetry_mgr_start(void);
void telemetry_mgr_stop(void);
int telemetry_mgr_dump(void);

/* Isolated Metrics Lists Lifecycles */
telemetry_list_t* telemetry_list_create(const char *list_name);
void telemetry_list_destroy(telemetry_list_t *list);

/* Single Metrics Allocation and Management Routines (Returns 0 on success, -1 on error) */
int telemetry_alloc(telemetry_list_t *list, const char *name, telemetry_behavior_t flags, telemetry_entry_t *entry);
int telemetry_alloc_bulk(telemetry_list_t *list, const telemetry_bulk_init_t *stats_array, int stats_num);
void telemetry_free(telemetry_list_t *list, const char *name);
telemetry_entry_t* telemetry_find(const telemetry_list_t *list, const char *name);

/* Realtime Telemetry Mutators and Read Accessors */
uint64_t telemetry_update(telemetry_entry_t *entry, uint64_t value);
uint64_t telemetry_get_value(const telemetry_entry_t *entry);

#endif /* _LIBALGORITHMS_TELEMETRY_H */