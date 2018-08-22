#ifndef _SVC_STATISTICS_H
#define _SVC_STATISTICS_H
#include "kernel.h"
#include "svc_kernel/svc_status.h"

#define STATS_ENTRY_NAME_MAXSIZE 32

typedef void* stats_key;

//sum all values into one big integer (for example execution time of 100 sql statements)
#define STATS_TYPE_SUM 1
//update value in place - only last value is correct (for example last timestamp for changing value)
#define STATS_TYPE_LAST 2

KSTATUS statsStart(void);
void statsStop(void);
KSTATUS statsAlloc(const char* statsName, int type, stats_key *p_key);
void statsFree(stats_key key);
unsigned long long statsUpdate(stats_key key, unsigned long long value);
stats_key statsFind(const char* statsName);
unsigned long long statsGetValue(stats_key key);
#endif
