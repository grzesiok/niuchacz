#ifndef _LIBALGORITHMS_MEMORY_H
#define _LIBALGORITHMS_MEMORY_H
#define memoryPtrMove(ptr, diff) (void*)((unsigned long long)ptr+diff)
#define memoryAlign(val, alignment) ((val%alignment == 0) ? val : (val-(val%alignment)+alignment))
#endif
