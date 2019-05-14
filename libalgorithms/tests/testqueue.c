#include "../include/algorithms.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

typedef struct {
    pthread_t _id;
    perf_results_t _perfResults;
} test_thread_t;

typedef struct {
    perf_results_t _perfResults;
    volatile size_t _seq;
    size_t _numThreads;
    volatile size_t _array[50000000];
} test_cfg_t;

queue_t *gpQueue;
test_cfg_t gReadCfg;
test_cfg_t gWriteCfg;

size_t numElements = 3000000;
test_thread_t threadReadArray[3];
test_thread_t threadWriteArray[4];

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
    size_t ret, size;
    test_thread_t *threadCfg = (test_thread_t*)arg;
    perf_stats_t perfStats;
    perf_results_t perfResults;

    perfWatchThreadInit();
    perfWatchStart(&perfStats);
    queue_producer_new(gpQueue);
    while(1) {
        i = __atomic_add_fetch(&gWriteCfg._seq, 1, __ATOMIC_RELEASE)-1;
        if(i >= numElements)
            break;
        size = randomGet(sizeof(i), 1000);
        *((size_t*)buffer) = i;
        gWriteCfg._array[i] = size;
        ret = queue_write(gpQueue, buffer, size, NULL);
        if(ret != size){
            printf("WrongSize[WRITE, i=%zu, orig=%zu, curr=%zu]\n", i, size, ret);
        }
    }
    queue_producer_free(gpQueue);
    perfResults = perfWatchStop(&perfStats);
    perfWatchAdd(&threadCfg->_perfResults, &perfResults);
    return NULL;
}

void* routineRead(void* arg) {
    char buffer[1000];
    size_t readedI, readedSizeI;
    test_thread_t *threadCfg = (test_thread_t*)arg;
    perf_stats_t perfStats;
    perf_results_t perfResults;

    perfWatchThreadInit();
    perfWatchStart(&perfStats);
    queue_consumer_new(gpQueue);
    while(1) {
        if(__atomic_add_fetch(&gReadCfg._seq, 1, __ATOMIC_RELEASE)-1 >= numElements)
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
        if(readedSizeI != gWriteCfg._array[readedI]) {
            printf("WrongSize[READ, i=%zu, orig=%zu, curr=%zu]\n", readedI, gReadCfg._array[readedI], readedSizeI);
        }
        __atomic_add_fetch(&gReadCfg._array[readedI], 1, __ATOMIC_RELEASE);
    }
    queue_consumer_free(gpQueue);
    perfResults = perfWatchStop(&perfStats);
    perfWatchAdd(&threadCfg->_perfResults, &perfResults);
    return NULL;
}

int main() {
    time_t tt;
    int seed = time(&tt);
    srand(seed);

    // Configure test
    gReadCfg._numThreads = 3;
    gReadCfg._seq = 0;
    gWriteCfg._numThreads = 4;
    gWriteCfg._seq = 0;
    // End Configuration
    if(!perfWatchInit()) {
        perror("Error during init PAPI:\n");
        return 1;
    }
    size_t i;
    printf("Queue:CREATE\n");
    gpQueue = queue_create(1000000);
    for(i = 0;i < gWriteCfg._numThreads;i++) {
        if(pthread_create(&threadWriteArray[i]._id, NULL, routineWrite, &threadWriteArray[i])) {
            perror("Error creating thread\n");
            return 1;
        }
    }
    for(i = 0;i < gReadCfg._numThreads;i++) {
        if(pthread_create(&threadReadArray[i]._id, NULL, routineRead, &threadReadArray[i])) {
            perror("Error creating thread\n");
            return 1;
        }
    }
    printf("Queue: <==STATS_DUMP==>\n");
    printf("Queue: EntriesCurrent=%zu\n", gpQueue->_stats_EntriesCurrent);
    printf("Queue: EntriesMax=%zu\n", gpQueue->_stats_EntriesMax);
    printf("Queue: MemUsageCurrent=%zu\n", gpQueue->_stats_MemUsageCurrent);
    printf("Queue: MemUsageMax=%zu\n", gpQueue->_stats_MemUsageMax);
    printf("Queue: MemSizeCurrent=%zu\n", gpQueue->_stats_MemSizeCurrent);
    printf("Queue: MemSizeMin=%zu\n", gpQueue->_stats_MemSizeMin);
    printf("Queue: MemSizeMax=%zu\n", gpQueue->_stats_MemSizeMax);
    struct timespec ts;
    int ret;
    uint64_t cumulativeReadTime = 0, cumulativeWriteTime = 0;
    for(i = 0;i < gReadCfg._numThreads;i++) {
        do {
            if(ret == ETIMEDOUT) {
                //printf("actualReadI=%zu\n", actualReadI);
                //printf("actualWriteI=%zu\n", actualWriteI);
            }
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 1;
        } while((ret = pthread_timedjoin_np(threadReadArray[i]._id, NULL, &ts)) == ETIMEDOUT);
        cumulativeReadTime += threadReadArray[i]._perfResults._time;
    }
    for(i = 0;i < gWriteCfg._numThreads;i++) {
        pthread_tryjoin_np(threadWriteArray[i]._id, NULL);
        cumulativeWriteTime += threadWriteArray[i]._perfResults._time;
    }
    printf("Queue: <==STATS_DUMP==>\n");
    printf("Queue: EntriesCurrent=%zu\n", gpQueue->_stats_EntriesCurrent);
    printf("Queue: EntriesMax=%zu\n", gpQueue->_stats_EntriesMax);
    printf("Queue: MemUsageCurrent=%zu\n", gpQueue->_stats_MemUsageCurrent);
    printf("Queue: MemUsageMax=%zu\n", gpQueue->_stats_MemUsageMax);
    printf("Queue: MemSizeCurrent=%zu\n", gpQueue->_stats_MemSizeCurrent);
    printf("Queue: MemSizeMin=%zu\n", gpQueue->_stats_MemSizeMin);
    printf("Queue: MemSizeMax=%zu\n", gpQueue->_stats_MemSizeMax);
    printf("Queue:DESTROY\n");
    queue_destroy(gpQueue);
    for(i = 0;i < numElements;i++) {
        if(gReadCfg._array[i] != 1)
            printf("readArray[%zu] read %zu times insted once\n", i, gReadCfg._array[i]);
    }
    //timerTimespecDiff(&readTimeStart, &ReadTimeEnd, &ReadTime);
    //timerTimespecDiff(&writeTimeStart, &writeTimeEnd, &WriteTime);
    printf("WRITE[TIME in s]: %f\n", (float)cumulativeWriteTime/1000000000.0f/(float)gWriteCfg._numThreads);
    printf("WRITE[THROUPGHPUT in item/s]: %zu\n", (size_t)((float)numElements/((float)cumulativeWriteTime/1000000000.0f)));
    printf("READ[TIME in s]: %f\n", (float)cumulativeReadTime/1000000000.0f/(float)gReadCfg._numThreads);
    printf("READ[THROUPGHPUT in item/s]: %zu\n", (size_t)((float)numElements/((float)cumulativeReadTime/1000000000.0f)));
    return 0;
}
