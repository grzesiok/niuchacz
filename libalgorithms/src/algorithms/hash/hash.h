#ifndef _LIBALGORITHMS_ALGORITHMS_HASH_H
#define _LIBALGORITHMS_ALGORITHMS_HASH_H
#include <stddef.h>

typedef unsigned long long hashvalue64;
typedef unsigned int hashvalue32;

typedef hashvalue64 (*hash64)(unsigned char* data, size_t size, hashvalue64 seed);
typedef hashvalue32 (*hash32)(unsigned char* data, size_t size, hashvalue64 seed);

hashvalue64 hash64FNV1(unsigned char* data, size_t size, hashvalue64 seed);
hashvalue64 hash64FNV1a(unsigned char* data, size_t size, hashvalue64 seed);
hashvalue64 hash64Murmur(unsigned char* data, size_t size, hashvalue64 seed);

hashvalue32 hash32FNV1(unsigned char* data, size_t size, hashvalue32 seed);
hashvalue32 hash32FNV1a(unsigned char* data, size_t size, hashvalue32 seed);
hashvalue32 hash32Murmur(unsigned char* data, size_t size, hashvalue32 seed);
#endif
