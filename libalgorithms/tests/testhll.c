#include "../include/algorithms.h"
#include <stdio.h>

void estimateCardinality(unsigned int k, unsigned long long n) {
	PHYPERLOGLOG phll;
	unsigned long long i;
	unsigned long long estimatedCardinality;

	phll = hyperloglogCreate(hash64Murmur, k);
	for(i = 0;i < n;i++) {
		hyperloglogInsert(phll, (unsigned char*)&i, sizeof(i));
	}
	estimatedCardinality = hyperloglogCardinality(phll);
	printf("k=%u Cardinality=%llu EstimatedCardinality=%llu\n", k, n, estimatedCardinality);
	hyperloglogDestroy(phll);
}

int main() {
	unsigned long long i, k;

	for(k = 128;k <= 33000;k <<= 1) {
		for(i = 1;i < 100000000;i *= 10)
			estimateCardinality(k, i);
	}
	return 0;
}
