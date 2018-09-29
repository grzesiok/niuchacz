#ifndef _LIBALGORITHMS_ALGORITHMS_PERF_H
#define _LIBALGORITHMS_ALGORITHMS_PERF_H
#include <time.h>
#include <stdint.h>
#include "papi.h"

typedef struct {
    struct timespec _time;
} perf_stats_t;

typedef struct {
    uint64_t _time;
} perf_results_t;

void perfWatchStart(perf_stats_t *stats);
perf_results_t perfWatchStop(perf_stats_t *stats);
#endif /*_LIBALGORITHMS_ALGORITHMS_PERF_H */
