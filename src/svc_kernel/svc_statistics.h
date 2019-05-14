#ifndef _SVC_STATISTICS_H
#define _SVC_STATISTICS_H
#include "kernel.h"
#include "svc_kernel/svc_status.h"
#include "algorithms.h"

typedef struct {
    char* _name;
    art_tree _tree;
} stats_list_t;

typedef struct {
    int _flags;
    unsigned long long _value;
} stats_entry_t;

typedef struct {
    const char* _statsEntry_key;
    int _statsEntry_flags;
    stats_entry_t* _statsEntry_ptr;
} stats_bulk_init_t;

//sum all values into one big integer (for example execution time of 100 sql statements)
#define STATS_FLAGS_TYPE_SUM 1
//update value in place - only last value is correct (for example last timestamp for changing value)
#define STATS_FLAGS_TYPE_LAST 2

// Statistics Manager
KSTATUS statsmgrStart(void);
void statsmgrStop(void);
KSTATUS statsmgrDump(void);
// Statistics List
stats_list_t* statsCreate(const char* listName);
void statsDestroy(stats_list_t* p_stats);
// Statistics List Entries
KSTATUS statsAlloc(stats_list_t* p_stats, const char* statsName, int flags, stats_entry_t* p_entry);
KSTATUS statsAllocBulk(stats_list_t* p_stats, stats_bulk_init_t* p_stats_array, int stats_num);
void statsFree(stats_list_t* p_stats, const char* statsName);
stats_entry_t* statsFind(stats_list_t* p_stats, const char* statsName);
// General funcs
unsigned long long statsUpdate(stats_entry_t* p_entry, unsigned long long value);
unsigned long long statsGetValue(stats_entry_t* p_entry);
#endif
