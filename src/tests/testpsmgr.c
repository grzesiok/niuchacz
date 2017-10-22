#include "../psmgr/psmgr.h"

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
	pentry->_p_private_mem = malloc(5);
	printf("testRoutine START(%s)...\n", pentry->_key);
	int a = 1/0;
	sleep(1);
	printf("testRoutine STOP(%s)...\n", pentry->_key);
	return 1;
}

void cancelRoutine(void* p_arg) {
	PTEST_ENTRY pentry = (PTEST_ENTRY)p_arg;
	printf("cancelRoutine START(%s)...\n", pentry->_key);
	free(pentry->_p_private_mem);
	printf("cancelRoutine STOP(%s)...\n", pentry->_key);
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

	printf("PSMGR Starting...\n");
	_status = psmgrStart();
	for(i = 0;i < sizeof(gTab)/sizeof(gTab[0]);i++) {
		_status = psmgrCreateThread(gTab[i]._key, gTab[i]._p_execRoutine, gTab[i]._p_cancelRoutine, &gTab[i]);
	}
	printf("PSMGR Stopping...\n");
	psmgrStop();
	return 0;
}