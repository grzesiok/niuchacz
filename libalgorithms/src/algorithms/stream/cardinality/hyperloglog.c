#include "hyperloglog.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "../../../../include/memory.h"

//internal API
static unsigned char i_hyperloglogGetRightmost0seqBits(unsigned long long val, unsigned char bitfrom, unsigned char nbits) {
	unsigned char i;
    for(i = bitfrom;i < nbits;i++) {
        if((val & 1) == 1)
            return i - bitfrom + 1;
        val >>= 1;
    }
    return (nbits - bitfrom) + 1;
}

static double i_hyperloglogGetAlpha(unsigned int k) {
	switch(k) {
	case 16:
		return 0.673f;
	case 32:
		return 0.697f;
	case 64:
		return 0.709f;
	}
	return 0.7213 / (1.0 + 1.079 / (float) k);
}

static double i_hyperloglogLinearCounting(double m, double V) {
	return m * log(m / V);
}

//external API
PHYPERLOGLOG hyperloglogCreate(hash64 func, unsigned int k) {
	PHYPERLOGLOG phll = (PHYPERLOGLOG)malloc(sizeof(HYPERLOGLOG)+k*sizeof(unsigned char));
	if(phll == NULL)
		return NULL;
	phll->_k = k;
	phll->_bits = log2(k);
	phll->_func = func;
	phll->_R = (unsigned char*)memoryPtrMove(phll, sizeof(HYPERLOGLOG));
	memset(phll->_R, 0, k*sizeof(unsigned char));
	return phll;
}

void hyperloglogDestroy(PHYPERLOGLOG phll) {
	free(phll);
}

bool hyperloglogInsert(PHYPERLOGLOG phll, unsigned char* pdata, size_t size) {
	hashvalue64 hashValue = phll->_func(pdata, size, 0);
	hashvalue64 x = hashValue % phll->_k;
	unsigned char b = i_hyperloglogGetRightmost0seqBits(hashValue >> phll->_bits, 0, sizeof(hashvalue64));
	if(phll->_R[x] < b) {
		phll->_R[x] = b;
	}
	return true;
}

unsigned long long hyperloglogCardinality(PHYPERLOGLOG phll) {
    double Z = 0.0;
    double k = (double)phll->_k;
	unsigned short i;
	unsigned short zeroRegisters = 0;
    for(i = 0; i < phll->_k; i++) {
    	Z += 1.0 / (1 << phll->_R[i]);
    	if(phll->_R[i] == 0)
    		zeroRegisters++;
    }
    double estimate = i_hyperloglogGetAlpha(phll->_k) * k * k * (1 / Z);
    if(estimate <= (5.0/2.0)*k) {
    	if(zeroRegisters == 0)
    		return estimate;
    	else return i_hyperloglogLinearCounting(k, zeroRegisters);
    } else {
    	return estimate;
    }
}

bool hyperloglogMerge(PHYPERLOGLOG phll, PHYPERLOGLOG phll2) {
	if(phll->_k != phll2->_k)
		return false;
	unsigned int i;
	for(i = 0;i < phll->_k;i++) {
		if(phll->_R[i] < phll2->_R[i]) {
			phll->_R[i] = phll2->_R[i];
		}
	}
	return true;
}
