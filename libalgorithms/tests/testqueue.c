#include "../include/algorithms.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

queue_t *gpQueue;
size_t numElements = 3000000;
volatile size_t readArray[50000000];
volatile size_t writeArray[50000000];
size_t numReadThreads = 3;
pthread_t threadReadArray[3];
size_t numWriteThreads = 4;
pthread_t threadWriteArray[4];
volatile size_t actualReadI;
volatile size_t actualWriteI;

int randomGet(int min, int max) {
    int tmp;
    if (max>=min)
        max-= min;
    else {
        tmp= min - max;
        min= max;
        max= tmp;
    }
    return max ? (rand() % max + min) : min;
}

void* routineWrite(void* arg) {
    char buffer[1000];
    size_t i = 0;
    size_t ret, size, writeTime = 0;
    struct timespec startTime;

    timerWatchStart(&startTime);
    queue_producer_new(gpQueue);
    while(1) {
        i = __atomic_add_fetch(&actualWriteI, 1, __ATOMIC_RELEASE);
        if(i >= numElements)
            break;
        size = randomGet(sizeof(i), 1000);
        *((size_t*)buffer) = i;
        writeArray[i] = size;
        ret = queue_write(gpQueue, buffer, size, NULL);
        if(ret != size){
            printf("WrongSize[WRITE, i=%zu, orig=%zu, curr=%zu]\n", i, size, ret);
        }
    }
    queue_producer_free(gpQueue);
    writeTime += timerWatchStop(startTime);
    return (void*)writeTime;
}

void* routineRead(void* arg) {
    char buffer[1000];
    size_t readedI, readedSizeI, readTime = 0;
    struct timespec startTime;

    timerWatchStart(&startTime);
    queue_consumer_new(gpQueue);
    while(1) {
        if(__atomic_add_fetch(&actualReadI, 1, __ATOMIC_RELEASE) >= numElements)
            break;
        readedSizeI = queue_read(gpQueue, buffer, NULL);
        if(readedSizeI == -1) {
            printf("readedSizeI=%zu\n", readedSizeI);
            continue;
        }
        readedI = *((size_t*)buffer);
        if(readedI >= numElements) {
            printf("readedI=%zu\n", readedI);
        }
        if(readedSizeI != writeArray[readedI]) {
            printf("WrongSize[READ, i=%zu, orig=%zu, curr=%zu]\n", readedI, writeArray[readedI], readedSizeI);
        }
        __atomic_add_fetch(&readArray[readedI], 1, __ATOMIC_RELEASE);
    }
    queue_consumer_free(gpQueue);
    readTime += timerWatchStop(startTime);
    return (void*)readTime;
}

int main() {
    time_t tt;
    int seed = time(&tt);
    srand(seed);

    actualReadI = actualWriteI = 0;

	size_t i;
	printf("Queue:CREATE\n");
	gpQueue = queue_create(1000000);
	for(i = 0;i < numWriteThreads;i++) {
		if(pthread_create(&threadWriteArray[i], NULL, routineWrite, NULL)) {
			perror("Error creating thread\n");
			return 1;
		}
	}
	for(i = 0;i < numReadThreads;i++) {
		if(pthread_create(&threadReadArray[i], NULL, routineRead, NULL)) {
			perror("Error creating thread\n");
			return 1;
		}
	}
	struct timespec ts;
	int ret;
	size_t cumulativeReadTime = 0, cumulativeWriteTime = 0;
	size_t readTime = 0, writeTime = 0;
	for(i = 0;i < numReadThreads;i++) {
		do {
			if(ret == ETIMEDOUT) {
				//printf("actualReadI=%zu\n", actualReadI);
				//printf("actualWriteI=%zu\n", actualWriteI);
			}
			clock_gettime(CLOCK_REALTIME, &ts);
			ts.tv_sec += 1;
		} while((ret = pthread_timedjoin_np(threadReadArray[i], (void**)&readTime, &ts)) == ETIMEDOUT);
		cumulativeReadTime += readTime;
	}
	for(i = 0;i < numWriteThreads;i++) {
		pthread_tryjoin_np(threadWriteArray[i], (void**)&writeTime);
		cumulativeWriteTime += writeTime;
	}
	printf("Queue:DESTROY\n");
	queue_destroy(gpQueue);
	for(i = 1;i <= numElements;i++) {
		if(readArray[i] != 1)
			printf("readArray[%zu] read %zu times insted once\n", i, readArray[i]);
	}
	//timerTimespecDiff(&readTimeStart, &ReadTimeEnd, &ReadTime);
	//timerTimespecDiff(&writeTimeStart, &writeTimeEnd, &WriteTime);
	printf("WRITE[TIME in s]: %f\n", (float)cumulativeWriteTime/1000000000.0f/(float)numWriteThreads);
	printf("WRITE[THROUPGHPUT in item/s]: %zu\n", (size_t)((float)numElements/((float)cumulativeWriteTime/1000000000.0f)));
	printf("READ[TIME in s]: %f\n", (float)cumulativeReadTime/1000000000.0f/(float)numReadThreads);
	printf("READ[THROUPGHPUT in item/s]: %zu\n", (size_t)((float)numElements/((float)cumulativeReadTime/1000000000.0f)));
	return 0;
}
