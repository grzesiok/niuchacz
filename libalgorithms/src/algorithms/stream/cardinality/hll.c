#include "hll.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"

/* Hidden structural composition details layout */
struct hll_t {
    unsigned int registers_count; /* Number of registers (m = 2^p) */
    unsigned char p_bits;         /* Precision bits used for register indexing */
    hash64 hash_func;             /* 64-bit hashing algorithm reference */
    unsigned char *registers;     /* Contiguous byte block for max rank storage */
};

// ====================================================================
// INTERNAL API: MATHEMATICAL AUXILIARY ESTIMATORS
// ====================================================================

/* Scan bits to discover the index position of the first set bit (1-based rank) */
static unsigned char i_hll_get_rho(uint64_t hash_value, unsigned char max_bits) {
    unsigned char rho = 1;
    while (rho <= max_bits && (hash_value & 1) == 0) {
        rho++;
        hash_value >>= 1;
    }
    return rho;
}

/* Retrieve Flajolet alpha correction factors corresponding to precision bounds */
static double i_hll_get_alpha(unsigned int m) {
    switch (m) {
        case 16:  return 0.673;
        case 32:  return 0.697;
        case 64:  return 0.709;
        default:  return 0.7213 / (1.0 + 1.079 / (double)m);
    }
}

/* Fallback counting approximation for low-range sparse data sets */
static double i_hll_linear_counting(double m, double empty_registers) {
    return m * log(m / empty_registers);
}

// ====================================================================
// EXTERNAL API: LIFECYCLE AND PROCESSING HOOKS
// ====================================================================

hll_t* hll_create(hash64 hash_func, unsigned int precision_bits) {
    /* Validation: Standard HyperLogLog precision bounds are strictly between 4 and 16 */
    if (hash_func == NULL || precision_bits < 4 || precision_bits > 16)
        return NULL;

    unsigned int m = 1U << precision_bits;
    hll_t *hll = (hll_t*)malloc(sizeof(hll_t) + (m * sizeof(unsigned char)));
    if (hll == NULL)
        return NULL;

    hll->registers_count = m;
    hll->p_bits = precision_bits;
    hll->hash_func = hash_func;
    
    /* Dynamically shift target pointers past the structural fields */
    hll->registers = (unsigned char*)memoryPtrMove(hll, sizeof(hll_t));
    memset(hll->registers, 0, m * sizeof(unsigned char));

    return hll;
}

void hll_destroy(hll_t *hll) {
    free(hll);
}

bool hll_insert(hll_t *hll, const unsigned char *pdata, size_t size) {
    if (hll == NULL || pdata == NULL)
        return false;

    /* Compute raw 64-bit non-cryptographic signature hash */
    uint64_t hash_value = hll->hash_func(pdata, size, 0);

    /* FIXED: Extract register index from the upper p-bits to satisfy standard partition mechanics */
    unsigned int register_idx = hash_value >> (64 - hll->p_bits);

    /* FIXED: Evaluate Rho on the remaining bits (64 - p) ensuring 64-bit limits are passed */
    uint64_t remaining_hash = hash_value & (~0ULL >> hll->p_bits);
    unsigned char rho = i_hll_get_rho(remaining_hash, 64 - hll->p_bits);

    if (hll->registers[register_idx] < rho) {
        hll->registers[register_idx] = rho;
    }

    return true;
}

unsigned long long hll_cardinality(const hll_t *hll) {
    if (hll == NULL)
        return 0;

    double sum = 0.0;
    unsigned int empty_registers = 0;

    for (unsigned int i = 0; i < hll->registers_count; i++) {
        /* FIXED: Replaced unsafe shift operation with robust mathematical floating-point powers */
        sum += 1.0 / pow(2.0, hll->registers[i]);
        if (hll->registers[i] == 0) {
            empty_registers++;
        }
    }

    double m = (double)hll->registers_count;
    double estimate = i_hll_get_alpha(hll->registers_count) * m * m * (1.0 / sum);

    /* Apply small-range sub-algorithm corrections if required */
    if (estimate <= (2.5 * m)) {
        if (empty_registers > 0) {
            return (unsigned long long)round(i_hll_linear_counting(m, (double)empty_registers));
        }
    }
    return (unsigned long long)round(estimate);
}

bool hll_merge(hll_t *hll_dst, const hll_t *hll_src) {
    if (hll_dst == NULL || hll_src == NULL || hll_dst->registers_count != hll_src->registers_count)
        return false;

    for (unsigned int i = 0; i < hll_dst->registers_count; i++) {
        if (hll_dst->registers[i] < hll_src->registers[i]) {
            hll_dst->registers[i] = hll_src->registers[i];
        }
    }
    return true;
}