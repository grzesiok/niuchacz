#ifndef _ALGORITHMS_STREAM_HLL_H
#define _ALGORITHMS_STREAM_HLL_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../../hash/hash.h"

typedef struct HYPERLOGLOG {
	unsigned int _k;
	unsigned char _bits;
	hash64 _func;
	unsigned char *_R;
} HYPERLOGLOG, *PHYPERLOGLOG;

PHYPERLOGLOG hyperloglogCreate(hash64 func, unsigned int k);
void hyperloglogDestroy(PHYPERLOGLOG phll);
bool hyperloglogInsert(PHYPERLOGLOG phll, unsigned char* pdata, size_t size);
bool hyperloglogMerge(PHYPERLOGLOG phll, PHYPERLOGLOG phll2);
unsigned long long hyperloglogCardinality(PHYPERLOGLOG phll);
#endif
