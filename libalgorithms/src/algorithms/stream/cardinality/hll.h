#ifndef _LIBALGORITHMS_ALGORITHMS_HYPERLOGLOG_H
#define _LIBALGORITHMS_ALGORITHMS_HYPERLOGLOG_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "algorithms/hash/hash.h"

/* 
 * Opaque pointer definition for the HyperLogLog descriptor.
 * Wewnętrzna struktura rejestrów jest całkowicie ukryta w pliku .c.
 */
typedef struct hll_t hll_t;

/* Public API Management Lifecycles */
hll_t* hll_create(hash64 hash_func, unsigned int precision_bits);
void hll_destroy(hll_t *hll);

/* Public API Data Processing Subsystems */
bool hll_insert(hll_t *hll, const unsigned char *pdata, size_t size);
bool hll_merge(hll_t *hll_dst, const hll_t *hll_src);
unsigned long long hll_cardinality(const hll_t *hll);

#endif /* _LIBALGORITHMS_ALGORITHMS_HYPERLOGLOG_H */