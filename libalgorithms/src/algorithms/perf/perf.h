#ifndef _LIBALGORITHMS_ALGORITHMS_PERF_H
#define _LIBALGORITHMS_ALGORITHMS_PERF_H
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include "papi.h"

typedef struct {
    bool _isActive;
    struct timespec _time;
} perf_stats_t;

typedef struct {
    long_long _PAPIcode;
    const char* _PAPIdesc;
} perf_event_t;

typedef struct {
    uint64_t _time;
    long_long _CPUEvents[64];
} perf_results_t;

bool perfWatchInit(void);
bool perfWatchThreadInit(void);
void perfWatchStart(perf_stats_t *stats);
perf_results_t perfWatchStop(perf_stats_t *stats);
void perfWatchAdd(perf_results_t *pdstStats, perf_results_t *psrcStats);
void perfPrintf(perf_results_t *stats);
#endif /*_LIBALGORITHMS_ALGORITHMS_PERF_H */
