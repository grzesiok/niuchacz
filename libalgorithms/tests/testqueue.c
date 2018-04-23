#include "../include/algorithms.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

queue_t *gpQueue;
size_t numElements = 1000000000;
size_t numThreads = 4;
size_t readArray[1000000000];
pthread_t threadReadArray[4];
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

void timerTimespecDiff(struct timespec *start, struct timespec *stop, struct timespec *result) {
    if ((stop->tv_nsec - start->tv_nsec) < 0) {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    } else {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }
}

size_t timerTimespecToLongLongNs(struct timespec currentTime) {
    return currentTime.tv_sec*1000000000 + currentTime.tv_nsec;
}

void* routineWrite(void* arg) {
	size_t i = 0;
	struct timespec writeTimeStart, writeTimeEnd, writeTime;
	clock_gettime(CLOCK_REALTIME, &writeTimeStart);
	while(1) {
		i = __atomic_fetch_add(&actualWriteI, 1, __ATOMIC_RELEASE);
		if(i >= numElements)
			break;
		queue_write(gpQueue, &i, sizeof(i), NULL);
	}
	clock_gettime(CLOCK_REALTIME, &writeTimeEnd);
	timerTimespecDiff(&writeTimeStart, &writeTimeEnd, &writeTime);
	return (void*)timerTimespecToLongLongNs(writeTime);
}

void* routineRead(void* arg) {
	size_t readedI, readedSizeI;
	struct timespec readTimeStart, readTimeEnd, readTime;
	clock_gettime(CLOCK_REALTIME, &readTimeStart);
	while(1) {
		if(__atomic_fetch_add(&actualReadI, 1, __ATOMIC_RELEASE) >= numElements)
			break;
		readedSizeI = queue_read(gpQueue, &readedI, NULL);
		if(readedI >= numElements) {
			printf("readedI=%zu\n", readedI);
		}
		if(readedSizeI != sizeof(size_t)) {
			printf("WrongSize[%zu]=%zu\n", readedI, readedSizeI);
		}
		__atomic_fetch_add(&readArray[readedI], 1, __ATOMIC_RELEASE);
	}
	clock_gettime(CLOCK_REALTIME, &readTimeEnd);
	timerTimespecDiff(&readTimeStart, &readTimeEnd, &readTime);
	return (void*)timerTimespecToLongLongNs(readTime);
}

int main() {
    time_t tt;
    int seed = time(&tt);
    srand(seed);

    actualReadI = actualWriteI = 0;

	size_t i;
	printf("Queue:CREATE\n");
	gpQueue = queue_create(1000000);
	for(i = 0;i < numThreads;i++) {
		if(pthread_create(&threadWriteArray[i], NULL, routineWrite, NULL)) {
			perror("Error creating thread\n");
			return 1;
		}
		if(pthread_create(&threadReadArray[i], NULL, routineRead, NULL)) {
			perror("Error creating thread\n");
			return 1;
		}
	}
	struct timespec ts;
	int ret;
	size_t cumulativeReadTime = 0, cumulativeWriteTime = 0;
	size_t readTime = 0, writeTime = 0;
	for(i = 0;i < numThreads;i++) {
		do {
			if(ret == ETIMEDOUT) {
				printf("actualReadI=%zu\n", actualReadI);
				printf("actualWriteI=%zu\n", actualWriteI);
			}
			clock_gettime(CLOCK_REALTIME, &ts);
			ts.tv_sec += 1;
		} while((ret = pthread_timedjoin_np(threadReadArray[i], (void**)&readTime, &ts)) == ETIMEDOUT);
		cumulativeReadTime += readTime;
	}
	for(i = 0;i < numThreads;i++) {
		pthread_tryjoin_np(threadWriteArray[i], (void**)&writeTime);
		cumulativeWriteTime += writeTime;
	}
	printf("Queue:DESTROY\n");
	queue_destroy(gpQueue);
	for(i = 0;i < numElements;i++) {
		if(readArray[i] != 1)
			printf("readArray[%zu] read %zu times insted once\n", i, readArray[i]);
	}
	//timerTimespecDiff(&readTimeStart, &ReadTimeEnd, &ReadTime);
	//timerTimespecDiff(&writeTimeStart, &writeTimeEnd, &WriteTime);
	printf("WRITE[TIME in s]: %f\n", (float)cumulativeWriteTime/1000000000.0f/(float)numThreads);
	printf("WRITE[THROUPGHPUT in item/s]: %zu\n", (size_t)((float)numElements/((float)cumulativeWriteTime/1000000000.0f)));
	printf("READ[TIME in s]: %f\n", (float)cumulativeReadTime/1000000000.0f/(float)numThreads);
	printf("READ[THROUPGHPUT in item/s]: %zu\n", (size_t)((float)numElements/((float)cumulativeReadTime/1000000000.0f)));
	return 0;
}
