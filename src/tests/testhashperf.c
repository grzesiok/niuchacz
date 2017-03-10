#include <openssl/md2.h>
#include <openssl/md4.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/whrlpool.h>
#include <openssl/ripemd.h>
#include "../svc_kernel/svc_time.h"
#include <stdio.h>
#include <string.h>
#include "../../libalgorithms/include/hash.h"

#define testCryptHASH(NAME, TYPE, LENGTH, INIT, UPDATE, FINAL)\
unsigned long long testCrypt##NAME(int n, int size) {\
	char buffer[size];\
	TYPE c;\
	unsigned char result[LENGTH];\
	int i;\
	struct timespec startTime = timerStart();\
	for(i = 0;i < n;i++) {\
		INIT(&c);\
		UPDATE(&c, buffer, size);\
		FINAL((unsigned char*)result, &c);\
	}\
	return timerStop(startTime);\
}
#define testHASH64(NAME, FUNC)\
unsigned long long test##NAME(int n, int size) {\
	unsigned char buffer[size];\
	int i;\
	struct timespec startTime = timerStart();\
	for(i = 0;i < n;i++) {\
		FUNC((unsigned char*)buffer, size, 0);\
	}\
	return timerStop(startTime);\
}

testCryptHASH(MD2, MD2_CTX, MD2_DIGEST_LENGTH, MD2_Init, MD2_Update, MD2_Final)
testCryptHASH(MD4, MD4_CTX, MD4_DIGEST_LENGTH, MD4_Init, MD4_Update, MD4_Final)
testCryptHASH(MD5, MD5_CTX, MD5_DIGEST_LENGTH, MD5_Init, MD5_Update, MD5_Final)
testCryptHASH(SHA, SHA_CTX, SHA_DIGEST_LENGTH, SHA_Init, SHA_Update, SHA_Final)
testCryptHASH(SHA1, SHA_CTX, SHA_DIGEST_LENGTH, SHA1_Init, SHA1_Update, SHA1_Final)
testCryptHASH(SHA224, SHA256_CTX, SHA256_DIGEST_LENGTH, SHA224_Init, SHA224_Update, SHA224_Final)
testCryptHASH(SHA256, SHA256_CTX, SHA256_DIGEST_LENGTH, SHA256_Init, SHA256_Update, SHA256_Final)
testCryptHASH(SHA384, SHA512_CTX, SHA512_DIGEST_LENGTH, SHA384_Init, SHA384_Update, SHA384_Final)
testCryptHASH(SHA512, SHA512_CTX, SHA512_DIGEST_LENGTH, SHA512_Init, SHA512_Update, SHA512_Final)
testCryptHASH(WHIRLPOOL, WHIRLPOOL_CTX, WHIRLPOOL_DIGEST_LENGTH, WHIRLPOOL_Init, WHIRLPOOL_Update, WHIRLPOOL_Final)
testCryptHASH(RIPEMD160, RIPEMD160_CTX, RIPEMD160_DIGEST_LENGTH, RIPEMD160_Init, RIPEMD160_Update, RIPEMD160_Final)
testHASH64(FNV1, hash64FNV1)
testHASH64(FNV1a, hash64FNV1a)
testHASH64(Murmur, hash64Murmur)

#define doCryptTEST(NAME, SIZE)\
		unsigned long long timeValue##NAME = testCrypt##NAME(i, SIZE);\
		printf("%s;%d;%d;%llu;%f\n", #NAME, i, SIZE, timeValue##NAME, ns_to_s(timeValue##NAME));

#define doTEST64(NAME, SIZE)\
		unsigned long long timeValue##NAME = test##NAME(i, SIZE);\
		printf("%s;%d;%d;%llu;%f\n", #NAME, i, SIZE, timeValue##NAME, ns_to_s(timeValue##NAME));

double ns_to_s(unsigned long long nanoseconds) {
	double ns = (double)nanoseconds;
	return ns/1.0e9;
}

#define testDIST64(NAME)\
void printHashDist##NAME(unsigned long long hashTableSize, unsigned long long n) {\
	unsigned char hashTable[hashTableSize];\
	int i;\
	memset(hashTable, 0, sizeof(hashTable));\
	for(i = 0;i < n;i++) {\
		hashvalue64 hashValue = hash64##NAME((unsigned char*)&i, sizeof(int), 0);\
		hashTable[hashValue%hashTableSize]++;\
		if(hashTable[hashValue%hashTableSize] > 9)\
			hashTable[hashValue%hashTableSize] = 9;\
	}\
	double harmonicMean = 0.0f;\
	printf("hashTable(%6s,%3llu,%5llu)=[", #NAME, hashTableSize, n);\
	unsigned long long hashTableNonZero = 0;\
	for(i = 0;i < hashTableSize;i++) {\
		if(hashTable[i] != 0) {\
			harmonicMean += (1.0f/(double)hashTable[i]);\
			hashTableNonZero++;\
		}\
		printf("%u", hashTable[i]);\
	}\
	harmonicMean = 1.0f/(harmonicMean/(double)hashTableNonZero);\
	printf("]=%.3f(%llu)\n", harmonicMean, hashTableNonZero);\
}

testDIST64(Murmur)
testDIST64(FNV1)
testDIST64(FNV1a)

int main(int argc, char* argv[]) {
	int i, j;

	if(strcmp(argv[1], "dist") == 0) {
		for(i = 1;i <= 10000;i*=2) {
			printHashDistFNV1(127, i);
			printHashDistFNV1a(127, i);
			printHashDistMurmur(127, i);
		}
	} else if(strcmp(argv[1], "perf") == 0) {
		for(i = 1;i <= 10000;i*=10) {
			for(j = 16;j < 16000;j *= 16) {
				doCryptTEST(MD2, j);
				doCryptTEST(MD4, j);
				doCryptTEST(MD5, j);
				doCryptTEST(SHA, j);
				doCryptTEST(SHA1, j);
				doCryptTEST(SHA224, j);
				doCryptTEST(SHA256, j);
				doCryptTEST(SHA384, j);
				doCryptTEST(SHA512, j);
				doCryptTEST(WHIRLPOOL, j);
				doCryptTEST(RIPEMD160, j);
				doTEST64(FNV1, j);
				doTEST64(FNV1a, j);
				doTEST64(Murmur, j);
			}
		}
	}
	return 0;
}
