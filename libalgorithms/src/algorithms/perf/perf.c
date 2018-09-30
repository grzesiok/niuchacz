#include "perf.h"
#include "../timer/timer.h"

const int gcPAPIEvents[] = {
    //Conditional Branching
    PAPI_BR_CN,//Conditional branch instructions
    PAPI_BR_INS,//Branch instructions
    PAPI_BR_MSP,//Conditional branch instructions mispredicted
    PAPI_BR_NTK,//Conditional branch instructions not taken
    PAPI_BR_PRC,//Conditional branch instructions correctly predicted
    PAPI_BR_TKN,//Conditional branch instructions taken
    PAPI_BR_UCN,//Unconditional branch instructions
    PAPI_BRU_IDL,//Cycles branch units are idle
    PAPI_BTAC_M,//Branch target address cache misses
    //Cache Requests:
    PAPI_CA_CLN,//Requests for exclusive access to clean cache line
    PAPI_CA_INV,//Requests for cache line invalidation
    PAPI_CA_ITV,//Requests for cache line intervention
    PAPI_CA_SHR,//Requests for exclusive access to shared cache line
    PAPI_CA_SNP,//Requests for a snoop
    //Conditional Store:
    PAPI_CSR_FAL,//Failed store conditional instructions
    PAPI_CSR_SUC,//Successful store conditional instructions
    PAPI_CSR_TOT,//Total store conditional instructions
    //Floating Point Operations:
    PAPI_FAD_INS,//Floating point add instructions
    PAPI_FDV_INS,//Floating point divide instructions
    PAPI_FMA_INS,//FMA instructions completed
    PAPI_FML_INS,//Floating point multiply instructions
    PAPI_FNV_INS,//Floating point inverse instructions
    PAPI_FP_INS,//Floating point instructions
    PAPI_FP_OPS,//Floating point operations
    PAPI_FP_STAL,//Cycles the FP unit
    PAPI_FPU_IDL,//Cycles floating point units are idle
    PAPI_FSQ_INS,//Floating point square root instructions
    //Instruction Counting:
    PAPI_FUL_CCY,//Cycles with maximum instructions completed
    PAPI_FUL_ICY,//Cycles with maximum instruction issue
    PAPI_FXU_IDL,//Cycles integer units are idle
    PAPI_HW_INT,//Hardware interrupts
    PAPI_INT_INS,//Integer instructions
    PAPI_TOT_CYC,//Total cycles
    PAPI_TOT_IIS,//Instructions issued
    PAPI_TOT_INS,//Instructions completed
    PAPI_VEC_INS,//Vector/SIMD instructions
    //Cache Access:
    PAPI_L1_DCA,//L1 data cache accesses
    PAPI_L1_DCH,//L1 data cache hits
    PAPI_L1_DCM,//L1 data cache misses
    PAPI_L1_DCR,//L1 data cache reads
    PAPI_L1_DCW,//L1 data cache writes
    PAPI_L1_ICA,//L1 instruction cache accesses
    PAPI_L1_ICH,//L1 instruction cache hits
    PAPI_L1_ICM,//L1 instruction cache misses
    PAPI_L1_ICR,//L1 instruction cache reads
    PAPI_L1_ICW,//L1 instruction cache writes
    PAPI_L1_LDM,//L1 load misses
    PAPI_L1_STM,//L1 store misses
    PAPI_L1_TCA,//L1 total cache accesses
    PAPI_L1_TCH,//L1 total cache hits
    PAPI_L1_TCM,//L1 total cache misses
    PAPI_L1_TCR,//L1 total cache reads
    PAPI_L1_TCW,//L1 total cache writes
    PAPI_L2_DCA,//L2 data cache accesses
    PAPI_L2_DCH,//L2 data cache hits
    PAPI_L2_DCM,//L2 data cache misses
    PAPI_L2_DCR,//L2 data cache reads
    PAPI_L2_DCW,//L2 data cache writes
    PAPI_L2_ICA,//L2 instruction cache accesses
    PAPI_L2_ICH,//L2 instruction cache hits
    PAPI_L2_ICM,//L2 instruction cache misses
    PAPI_L2_ICR,//L2 instruction cache reads
    PAPI_L2_ICW,//L2 instruction cache writes
    PAPI_L2_LDM,//L2 load misses
    PAPI_L2_STM,//L2 store misses
    PAPI_L2_TCA,//L2 total cache accesses
    PAPI_L2_TCH,//L2 total cache hits
    PAPI_L2_TCM,//L2 total cache misses
    PAPI_L2_TCR,//L2 total cache reads
    PAPI_L2_TCW,//L2 total cache writes
    PAPI_L3_DCA,//L3 data cache accesses
    PAPI_L3_DCH,//L3 Data Cache Hits
    PAPI_L3_DCM,//L3 data cache misses
    PAPI_L3_DCR,//L3 data cache reads
    PAPI_L3_DCW,//L3 data cache writes
    PAPI_L3_ICA,//L3 instruction cache accesses
    PAPI_L3_ICH,//L3 instruction cache hits
    PAPI_L3_ICM,//L3 instruction cache misses
    PAPI_L3_ICR,//L3 instruction cache reads
    PAPI_L3_ICW,//L3 instruction cache writes
    PAPI_L3_LDM,//L3 load misses
    PAPI_L3_STM,//L3 store misses
    PAPI_L3_TCA,//L3 total cache accesses
    PAPI_L3_TCH,//L3 total cache hits
    PAPI_L3_TCM,//L3 cache misses
    PAPI_L3_TCR,//L3 total cache reads
    PAPI_L3_TCW,//L3 total cache writes
    //Data Access:
    PAPI_LD_INS,//Load instructions
    PAPI_LST_INS,//Load/store instructions completed
    PAPI_LSU_IDL,//Cycles load/store units are idle
    PAPI_MEM_RCY,//Cycles Stalled Waiting for memory Reads
    PAPI_MEM_SCY,//Cycles Stalled Waiting for memory accesses
    PAPI_MEM_WCY,//Cycles Stalled Waiting for memory writes
    PAPI_PRF_DM,//Data prefetch cache misses
    PAPI_RES_STL,//Cycles stalled on any resource
    PAPI_SR_INS,//Store instructions
    PAPI_STL_CCY,//Cycles with no instructions completed
    PAPI_STL_ICY,//Cycles with no instruction issue
    PAPI_SYC_INS,//Synchronization instructions completed
    //TLB Operations:
    PAPI_TLB_DM,//Data translation lookaside buffer misses
    PAPI_TLB_IM,//Instruction translation lookaside buffer misses
    PAPI_TLB_SD,//Translation lookaside buffer shootdowns
    PAPI_TLB_TL //Total translation lookaside buffer misses
};

void perfWatchStart(perf_stats_t *stats) {
    timerWatchStart(&stats->_time);
    if(PAPI_start_counters(gcPAPIEvents, sizeof(gcPAPIEvents)/sizeof(int)) != PAPI_OK) {
        stats->_isActive = true;
    } else stats->_isActive = false;
}

perf_results_t perfWatchStop(perf_stats_t *stats) {
    perf_results_t res;
    int values[sizeof(gcPAPIEvents)/sizeof(int)];
    res._time = timerWatchStop(stats->_time);
    if(stats->_isActive && PAPI_read_counters(values, sizeof(values)/sizeof(int)) != PAPI_OK) {
        stats->_isActive = false;
        return res;
    }
    return res;
}

void perfPrintf(perf_stats_t *stats) {
}
