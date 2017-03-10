#include "../../include/hash.h"

//internal API
#define FNV_32_PRIME ((hashvalue32)0x01000193)
#define FNV_64_PRIME ((hashvalue64)0x100000001b3ULL)

extern unsigned int MurmurHash2_32(const void * key, int len, unsigned int seed);
extern unsigned long long MurmurHash2_64(const void * key, int len, unsigned long long seed);

//external API
hashvalue64 hash64FNV1(unsigned char* data, size_t size, hashvalue64 seed) {
	hashvalue64 hashValue = seed;
	while(size-- > 0) {
		hashValue *= FNV_64_PRIME;
		hashValue ^= (hashvalue64)*data++;
	}
	return hashValue;
}

hashvalue32 hash32FNV1(unsigned char* data, size_t size, hashvalue32 seed) {
	hashvalue32 hashValue = seed;
	while(size-- > 0) {
		hashValue *= FNV_32_PRIME;
		hashValue ^= (hashvalue32)*data++;
	}
	return hashValue;
}

hashvalue64 hash64FNV1a(unsigned char* data, size_t size, hashvalue64 seed) {
	hashvalue64 hashValue = seed;
	while(size-- > 0) {
		hashValue ^= (hashvalue64)*data++;
		hashValue *= FNV_64_PRIME;
	}
	return hashValue;
}

hashvalue32 hash32FNV1a(unsigned char* data, size_t size, hashvalue32 seed) {
	hashvalue32 hashValue = seed;
	while(size-- > 0) {
		hashValue ^= (hashvalue32)*data++;
		hashValue *= FNV_32_PRIME;
	}
	return hashValue;
}

hashvalue64 hash64Murmur(unsigned char* data, size_t size, hashvalue64 seed) {
	return MurmurHash2_64(data, size, seed);
}

hashvalue32 hash32Murmur(unsigned char* data, size_t size, hashvalue32 seed) {
	return MurmurHash2_32(data, size, seed);
}
