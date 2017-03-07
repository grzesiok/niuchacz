#include <openssl/md2.h>
#include <openssl/md4.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/whrlpool.h>
#include <openssl/ripemd.h>
#include "../svc_kernel/svc_time.h"
#include <stdio.h>

#define testHASH(NAME, TYPE, LENGTH, INIT, UPDATE, FINAL)\
unsigned long long test##NAME(int n, int size) {\
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

testHASH(MD2, MD2_CTX, MD2_DIGEST_LENGTH, MD2_Init, MD2_Update, MD2_Final)
testHASH(MD4, MD4_CTX, MD4_DIGEST_LENGTH, MD4_Init, MD4_Update, MD4_Final)
testHASH(MD5, MD5_CTX, MD5_DIGEST_LENGTH, MD5_Init, MD5_Update, MD5_Final)
testHASH(SHA, SHA_CTX, SHA_DIGEST_LENGTH, SHA_Init, SHA_Update, SHA_Final)
testHASH(SHA1, SHA_CTX, SHA_DIGEST_LENGTH, SHA1_Init, SHA1_Update, SHA1_Final)
testHASH(SHA224, SHA256_CTX, SHA256_DIGEST_LENGTH, SHA224_Init, SHA224_Update, SHA224_Final)
testHASH(SHA256, SHA256_CTX, SHA256_DIGEST_LENGTH, SHA256_Init, SHA256_Update, SHA256_Final)
testHASH(SHA384, SHA512_CTX, SHA512_DIGEST_LENGTH, SHA384_Init, SHA384_Update, SHA384_Final)
testHASH(SHA512, SHA512_CTX, SHA512_DIGEST_LENGTH, SHA512_Init, SHA512_Update, SHA512_Final)
testHASH(WHIRLPOOL, WHIRLPOOL_CTX, WHIRLPOOL_DIGEST_LENGTH, WHIRLPOOL_Init, WHIRLPOOL_Update, WHIRLPOOL_Final)
testHASH(RIPEMD160, RIPEMD160_CTX, RIPEMD160_DIGEST_LENGTH, RIPEMD160_Init, RIPEMD160_Update, RIPEMD160_Final)

#define doTEST(NAME, SIZE)\
		unsigned long long timeValue##NAME = test##NAME(i, SIZE);\
		printf("%s;%d;%d;%llu\n", #NAME, i, SIZE, timeValue##NAME);

double ns_to_s(unsigned long long nanoseconds) {
	double ns = (double)nanoseconds;
	return ns/1.0e9;
}

int main(void) {
	int i, j;
	unsigned long long timeValue;

	for(i = 1;i <= 10000;i*=10) {
		for(j = 16;j < 16000;j *= 16) {
			doTEST(MD2, j);
			doTEST(MD4, j);
			doTEST(MD5, j);
			doTEST(SHA, j);
			doTEST(SHA1, j);
			doTEST(SHA224, j);
			doTEST(SHA256, j);
			doTEST(SHA384, j);
			doTEST(SHA512, j);
			doTEST(WHIRLPOOL, j);
			doTEST(RIPEMD160, j);
		}
	}
	return 0;
}
