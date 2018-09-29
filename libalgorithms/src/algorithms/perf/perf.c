#include "perf.h"
#include "../timer/timer.h"

void perfWatchStart(perf_stats_t *stats) {
    timerWatchStart(&stats->_time);
}

perf_results_t perfWatchStop(perf_stats_t *stats) {
    perf_results_t res;
    res._time = timerWatchStop(stats->_time);
    return res;
}
