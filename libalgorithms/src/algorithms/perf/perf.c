#include "perf.h"
#include "../timer/timer.h"
#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <pthread.h>

const perf_event_t gcPerfEvents[] = {
    //Conditional Branching
    {PAPI_BR_CN,"Conditional branch instructions"},
    {PAPI_BR_INS,"Branch instructions"},
    {PAPI_BR_MSP,"Conditional branch instructions mispredicted"},
    {PAPI_BR_PRC,"Conditional branch instructions correctly predicted"},
    {PAPI_BR_TKN,"Conditional branch instructions taken"},
    //Cache Requests:
    {PAPI_CA_CLN,"Requests for exclusive access to clean cache line"},
    {PAPI_CA_INV,"Requests for cache line invalidation"},
    {PAPI_CA_SHR,"Requests for exclusive access to shared cache line"},
    //Floating Point Operations:
    {PAPI_FP_INS,"Floating point instructions"},
    {PAPI_FP_OPS,"Floating point operations"},
    {PAPI_FP_STAL,"Cycles the FP unit"},
    {PAPI_FPU_IDL,"Cycles floating point units are idle"},
    //Instruction Counting:
    {PAPI_HW_INT,"Hardware interrupts"},
    {PAPI_TOT_CYC,"Total cycles"},
    {PAPI_TOT_IIS,"Instructions issued"},
    {PAPI_TOT_INS,"Instructions completed"},
    {PAPI_VEC_INS,"Vector/SIMD instructions"},
    //Cache Access:
    {PAPI_L1_DCA,"L1 data cache accesses"},
    {PAPI_L1_DCH,"L1 data cache hits"},
    {PAPI_L1_DCM,"L1 data cache misses"},
    {PAPI_L1_ICA,"L1 instruction cache accesses"},
    {PAPI_L1_ICH,"L1 instruction cache hits"},
    {PAPI_L1_ICM,"L1 instruction cache misses"},
    {PAPI_L1_LDM,"L1 load misses"},
    {PAPI_L1_STM,"L1 store misses"},
    {PAPI_L2_DCA,"L2 data cache accesses"},
    {PAPI_L2_DCH,"L2 data cache hits"},
    {PAPI_L2_DCM,"L2 data cache misses"},
    {PAPI_L2_ICA,"L2 instruction cache accesses"},
    {PAPI_L2_ICH,"L2 instruction cache hits"},
    {PAPI_L2_ICM,"L2 instruction cache misses"},
    {PAPI_L2_LDM,"L2 load misses"},
    {PAPI_L2_STM,"L2 store misses"},
    {PAPI_L3_DCA,"L3 data cache accesses"},
    {PAPI_L3_DCH,"L3 Data Cache Hits"},
    {PAPI_L3_DCM,"L3 data cache misses"},
    {PAPI_L3_ICA,"L3 instruction cache accesses"},
    {PAPI_L3_ICH,"L3 instruction cache hits"},
    {PAPI_L3_ICM,"L3 instruction cache misses"},
    {PAPI_L3_LDM,"L3 load misses"},
    {PAPI_L3_STM,"L3 store misses"},
    //Data Access:
    {PAPI_LD_INS,"Load instructions"},
    {PAPI_LST_INS,"Load/store instructions completed"},
    {PAPI_MEM_SCY,"Cycles Stalled Waiting for memory accesses"},
    {PAPI_MEM_WCY,"Cycles Stalled Waiting for memory writes"},
    {PAPI_STL_CCY,"Cycles with no instructions completed"},
    {PAPI_STL_ICY,"Cycles with no instruction issue"},
    {PAPI_SYC_INS,"Synchronization instructions completed"},
    //TLB Operations:
    {PAPI_TLB_DM,"Data translation lookaside buffer misses"},
    {PAPI_TLB_IM,"Instruction translation lookaside buffer misses"}
};
int gcPAPIEvents[sizeof(gcPerfEvents)/sizeof(perf_event_t)];

bool perfWatchInit(void) {
    int i;
    for(i = 0;i < sizeof(gcPAPIEvents)/sizeof(int);i++) {
        gcPAPIEvents[i] = gcPerfEvents[i]._PAPIcode;
    }
    if(PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT)
        return false;
    return true;
}

bool perfWatchThreadInit(void) {
    return (PAPI_thread_init(pthread_self) != PAPI_OK);
}

void perfWatchStart(perf_stats_t *stats) {
    timerWatchStart(&stats->_time);
    if(PAPI_start_counters(gcPAPIEvents, sizeof(gcPAPIEvents)/sizeof(int)) != PAPI_OK) {
        stats->_isActive = true;
    } else stats->_isActive = false;
}

perf_results_t perfWatchStop(perf_stats_t *stats) {
    perf_results_t res;
    long_long values[sizeof(gcPAPIEvents)/sizeof(int)];
    res._time = timerWatchStop(stats->_time);
    if(stats->_isActive && PAPI_read_counters(values, sizeof(values)/sizeof(int)) != PAPI_OK) {
        stats->_isActive = false;
        return res;
    }
    return res;
}

void perfWatchAdd(perf_results_t *pdstStats, perf_results_t *psrcStats) {
    int i;
    pdstStats->_time += psrcStats->_time;
    for(i = 0;i < sizeof(pdstStats->_CPUEvents)/sizeof(pdstStats->_CPUEvents[0]);i++) {
        pdstStats->_CPUEvents[i] += psrcStats->_CPUEvents[i];
    }
}

void perfPrintf(perf_results_t *stats) {
    int i;
    printf("%s :%"PRIu64"\n", "Thread time", stats->_time);
    for(i = 0;i < sizeof(stats->_CPUEvents)/sizeof(stats->_CPUEvents[0]);i++) {
        printf("%s : %llu\n", gcPerfEvents[i]._PAPIdesc, stats->_CPUEvents[i]);
    }
}
