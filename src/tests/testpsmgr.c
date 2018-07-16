#include <stdio.h>
#include <time.h>
#include "../svc_kernel/psmgr/psmgr.h"

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

typedef struct _TEST_ENTRY
{
	const char* _key;
	int _value;
	psmgr_execRoutine _p_execRoutine;
	psmgr_cancelRoutine _p_cancelRoutine;
	void* _p_private_mem;
} TEST_ENTRY, *PTEST_ENTRY;

KSTATUS testRoutine(void* p_arg) {
        PTEST_ENTRY pentry = (PTEST_ENTRY)p_arg;
	DPRINTF(TEXT("testRoutine START(%s)..."), pentry->_key);
	pentry->_p_private_mem = malloc(5);
	sleep(pentry->_value);
	int a, *b = (int*)NULL;
        switch(randomGet(0, 2)) {
	case 0:
		DPRINTF(TEXT("SIGFPE"));
		a = 1/0;
	case 1:
		DPRINTF(TEXT("SIGFLT"));
		*b = 1;
	}
        free(pentry->_p_private_mem);
	DPRINTF(TEXT("testRoutine STOP(%s)..."), pentry->_key);
	return 1;
}

void cancelRoutine(void* p_arg) {
	PTEST_ENTRY pentry = (PTEST_ENTRY)p_arg;
	DPRINTF(TEXT("cancelRoutine START(%s)..."), pentry->_key);
	free(pentry->_p_private_mem);
	DPRINTF(TEXT("cancelRoutine STOP(%s)..."), pentry->_key);
}

TEST_ENTRY gTab[] = {
		{"TEST1", 1, testRoutine, cancelRoutine},
		{"TEST2", 2, testRoutine, cancelRoutine},
		{"TEST3", 3, testRoutine, cancelRoutine},
		{"TEST4", 4, testRoutine, cancelRoutine},
		{"TEST5", 5, testRoutine, cancelRoutine}
};

int main()
{
	KSTATUS _status;
	TEST_ENTRY val;
	unsigned long long size, i;
	struct timeval timestamp;
	time_t tt;
	int seed = time(&tt);
	srand(seed);
	DPRINTF(TEXT("PSMGR Starting..."));
	_status = psmgrStart();
	for(i = 0;i < sizeof(gTab)/sizeof(gTab[0]);i++) {
		_status = psmgrCreateThread(gTab[i]._key, gTab[i]._p_execRoutine, gTab[i]._p_cancelRoutine, &gTab[i]);
	}
	psmgrIdle(5);
	DPRINTF(TEXT("PSMGR Stopping..."));
	psmgrStop();
	return 0;
}
