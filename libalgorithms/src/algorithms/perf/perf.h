#ifndef _LIBALGORITHMS_ALGORITHMS_PERF_H
#define _LIBALGORITHMS_ALGORITHMS_PERF_H
#include <time.h>
#include <stdint.h>
#include "papi.h"

typedef struct {
    bool _isActive;
    struct timespec _time;
} perf_stats_t;

typedef struct {
    uint64_t _time;
    //Conditional Branching
    long_long br_cn;
    long_long br_ins;
    long_long br_msp;
    long_long br_prc;
    long_long br_tkn;
    //Cache Requests:
    long_long ca_cln;
    long_long ca_inv;
    long_long ca_shr;
    //Floating Point Operations:
    long_long fp_ins;
    long_long fp_ops;
    long_long fp_stal;
    long_long fpu_idl;
    //Instruction Counting:
    long_long hw_int;
    long_long tot_cyc;
    long_long tot_iis;
    long_long tot_ins;
    long_long vec_ins;
    //Cache Access:
    long_long l1_dca;
    long_long l1_dch;
    long_long l1_dcm;
    long_long l1_ica;
    long_long l1_ich;
    long_long l1_icm;
    long_long l1_ldm;
    long_long l1_stm;
    long_long l2_dca;
    long_long l2_dch;
    long_long l2_dcm;
    long_long l2_ica;
    long_long l2_ich;
    long_long l2_icm;
    long_long l2_ldm;
    long_long l2_stm;
    long_long l3_dca;
    long_long l3_dch;
    long_long l3_dcm;
    long_long l3_ica;
    long_long l3_ich;
    long_long l3_icm;
    long_long l3_ldm;
    long_long l3_stm;
    //Data Access:
    long_long ld_ins;
    long_long lst_ins;
    long_long mem_scy;
    long_long sr_ins;
    long_long stl_ccy;
    long_long stl_icy;
    long_long syc_ins;
    //TLB Operations:
    long_long tlb_dm;
    long_long tlb_im;
} perf_results_t;

void perfWatchStart(perf_stats_t *stats, int events[], int eventsNum);
perf_results_t perfWatchStop(perf_stats_t *stats);
void perfPrintf(perf_results_t *stats);
#endif /*_LIBALGORITHMS_ALGORITHMS_PERF_H */
