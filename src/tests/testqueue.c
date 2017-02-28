#include "../queuemgr/queuemgr.h"

typedef struct _TEST_ENTRY
{
	int _key;
	int _value;
} TEST_ENTRY;

void dump_memory(char* ptr, int size)
{
	DPRINTF("dump_memory(%p)[", ptr);
	while(size-- > 0)
	{
		DPRINTF("%02x", *ptr++);
	}
	DPRINTF("]\n");
}

int test1()
{
	printf("==================TEST1s\n");
	PQUEUE mainqueue = NULL;
	KSTATUS _status;
	TEST_ENTRY val;
	unsigned long long size, i;

	_status = queuemgr_create(&mainqueue, 128);
	if(!KSUCCESS(_status))
	{
		printf("Error during allocate queue = %u\n", _status);
		return 1;
	}
	for(i = 0;i < 3;i++)
	{
		val._key = i+'a';
		val._value = i;
		dump_memory(&val, sizeof(val));
		printf("enqueue[%c] = %d\n", val._key, val._value);
		_status = queuemgr_enqueue(mainqueue, &val, sizeof(val));
		if(!KSUCCESS(_status))
		{
			printf("Error during enqueue = %u\n", _status);
			return 1;
		}
	}
	for(i = 0;i < 3;i++)
	{
		queuemgr_dequeue(mainqueue, &val, &size);
		if(size != sizeof(val))
		{
			printf("Incorrect size of fetched message = %u\n", size);
			return 1;
		}
		dump_memory(&val, sizeof(val));
		printf("dequeue[%c] = %d\n", val._key, val._value);
	}
	queuemgr_destroy(mainqueue);
	return 0;
}

int test2()
{
	printf("==================TEST2\n");
	PQUEUE mainqueue = NULL;
	KSTATUS _status;
	TEST_ENTRY val;
	unsigned long long size, i;

	_status = queuemgr_create(&mainqueue, 128);
	if(!KSUCCESS(_status))
	{
		printf("Error during allocate queue = %u\n", _status);
		return 1;
	}
	for(i = 0;i < 4;i++)
	{
		val._key = i+'a';
		val._value = i;
		dump_memory(&val, sizeof(val));
		printf("enqueue[%c] = %d\n", val._key, val._value);
		_status = queuemgr_enqueue(mainqueue, &val, sizeof(val));
		if((!KSUCCESS(_status) && i < 3) || (KSUCCESS(_status) && i == 3))
		{
			printf("Error during enqueue = %u\n", _status);
			return 1;
		}
	}
	queuemgr_destroy(mainqueue);
	return 0;
}

typedef struct _TEST_ENTRY31
{
	char _key;
	char _value[18];
} TEST_ENTRY31;

int test3()
{
	printf("==================TEST3\n");
	PQUEUE mainqueue = NULL;
	KSTATUS _status;
	TEST_ENTRY31 val;
	unsigned long long size;

	_status = queuemgr_create(&mainqueue, 148);
	if(!KSUCCESS(_status))
	{
		printf("Error during allocate queue = %u\n", _status);
		return 1;
	}
	val._key = 'a';
	strcpy(val._value, "First");
	printf("enqueue[%c] = %s\n", val._key, val._value);
	_status = queuemgr_enqueue(mainqueue, &val, sizeof(val));
	if(!KSUCCESS(_status))
	{
		printf("Error during enqueue = %u\n", _status);
		return 1;
	}
	val._key = 'b';
	strcpy(val._value, "Second");
	printf("enqueue[%c] = %s\n", val._key, val._value);
	_status = queuemgr_enqueue(mainqueue, &val, sizeof(val));
	if(!KSUCCESS(_status))
	{
		printf("Error during enqueue = %u\n", _status);
		return 1;
	}

	queuemgr_dequeue(mainqueue, &val, &size);
	if(size != sizeof(val))
	{
		printf("Incorrect size of fetched message = %u\n", size);
		return 1;
	}
	dump_memory(&val, sizeof(val));
	printf("dequeue[%c] = %s\n", val._key, val._value);
	val._key = 'c';
	strcpy(val._value, "Third");
	printf("enqueue[%c] = %s\n", val._key, val._value);
	_status = queuemgr_enqueue(mainqueue, &val, sizeof(val));
	int i;
	for(i = 0;i < 2;i++)
	{
		queuemgr_dequeue(mainqueue, &val, &size);
		if(size != sizeof(val))
		{
			printf("Incorrect size of fetched message = %u\n", size);
			return 1;
		}
		dump_memory(&val, sizeof(val));
		printf("dequeue[%c] = %s\n", val._key, val._value);
	}
	queuemgr_destroy(mainqueue);
	return 0;
}

int main()
{
	/*if(test1() != 0)
		return 1;
	if(test2() != 0)
		return 1;*/
	if(test3() != 0)
		return 1;
	return 0;
}
