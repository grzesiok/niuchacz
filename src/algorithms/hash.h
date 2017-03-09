#ifndef _ALGORITHMS_HASH_H
#define _ALGORITHMS_HASH_H
#include <stddef.h>

typedef unsigned long long hashvalue64;
typedef unsigned int hashvalue32;

hashvalue64 hash64FNV1(unsigned char* data, size_t size, hashvalue64 seed);
hashvalue64 hash64FNV1a(unsigned char* data, size_t size, hashvalue64 seed);
hashvalue64 hash64Murmur(unsigned char* data, size_t size, hashvalue64 seed);

hashvalue32 hash32FNV1(unsigned char* data, size_t size, hashvalue32 seed);
hashvalue32 hash32FNV1a(unsigned char* data, size_t size, hashvalue32 seed);
hashvalue32 hash32Murmur(unsigned char* data, size_t size, hashvalue32 seed);
#endif
