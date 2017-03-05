#include "kernel.h"
#include "svc_kernel/svc_kernel.h"
#include "database/database.h"
#include <openssl/md5.h>
#include <pcap.h>
#include <unistd.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include "queuemgr/queuemgr.h"

#define MAIN_THREAD_PRODUCER 0
#define MAIN_THREAD_CONSUMER 1

typedef struct _NIUCHACZ_CTX {
	char* _p_deviceName;
	stats_key _statsKey;
} NIUCHACZ_CTX, *PNIUCHACZ_CTX;

typedef struct _NIUCHACZ_MAIN {
	PQUEUE _p_framesQueue;
	NIUCHACZ_CTX _threads[2];
} NIUCHACZ_MAIN, *PNIUCHACZ_MAIN;

NIUCHACZ_MAIN g_Main;

void framesproducer_callback(const char* device, unsigned char *packet, struct timeval ts, unsigned int packet_len) {
	KSTATUS _status;

	_status = queuemgr_enqueue(g_Main._p_framesQueue, ts, packet, packet_len);
	if(!KSUCCESS(_status))
	{
		printf("Error in enqueue msg!\n");
	}
}

void framesconsumer_callback(const char* device, unsigned char *buffer) {
	MD5_CTX c;
	unsigned char packet_md5[MD5_DIGEST_LENGTH];
	char stmt[512];
	struct ether_header *eth_header;
	size_t packet_len;
	struct timeval timestamp;
	KSTATUS _status;

	_status = queuemgr_dequeue(g_Main._p_framesQueue, &timestamp, buffer, &packet_len);
	if(!KSUCCESS(_status))
	{
		printf("Error in dequeue msg!\n");
		return;
	}
	MD5_Init(&c);
    MD5_Update(&c, buffer, packet_len);
    MD5_Final((unsigned char*)packet_md5, &c);
    eth_header = (struct ether_header *) buffer;
    sprintf(stmt, "insert into frames (device, type, ts, len, hash) "
    			  "values (\"%s\", %u, %d.%06d, %zu, \"%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\")",
				  device,
				  ntohs(eth_header->ether_type),
				  (int) timestamp.tv_sec, (int) timestamp.tv_usec, packet_len,
				  packet_md5[0], packet_md5[1], packet_md5[2], packet_md5[3],
				  packet_md5[4], packet_md5[5], packet_md5[6], packet_md5[7],
				  packet_md5[8], packet_md5[9], packet_md5[10], packet_md5[11],
				  packet_md5[12], packet_md5[13], packet_md5[14], packet_md5[15]);
    database_exec(stmt);
}

void* pcapconsumer_thread_routine(void* arg)
{
	PNIUCHACZ_CTX p_ctx = (PNIUCHACZ_CTX)arg;
	unsigned char *buffer = MALLOC(unsigned char, 1024*1024);
	struct timespec startTime;

	while(svc_kernel_is_running()) {
		startTime = timerStart();
		framesconsumer_callback(p_ctx->_p_deviceName, buffer);
		statsUpdate(p_ctx->_statsKey, timerStop(startTime));
	}
	return NULL;
}

void* pcapproducer_thread_routine(void* arg)
{
	PNIUCHACZ_CTX p_ctx = (PNIUCHACZ_CTX)arg;
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_t *pcap;
	struct pcap_pkthdr header;
	unsigned char *packet;
	struct timespec startTime;

    printf("Listen on device=%s\n", p_ctx->_p_deviceName);
	pcap = pcap_open_live(p_ctx->_p_deviceName, BUFSIZ, 0, 1000, errbuf);
	if(pcap == NULL) {
		fprintf(stderr, "error reading pcap file: %s\n", errbuf);
		return NULL;
	}
	while(svc_kernel_is_running()) {
		packet = (unsigned char*)pcap_next(pcap, &header);
		if(packet != NULL)
		{
			startTime = timerStart();
			framesproducer_callback(p_ctx->_p_deviceName, packet, header.ts, header.caplen);
			statsUpdate(p_ctx->_statsKey, timerStop(startTime));
		}
	}
	pcap_close(pcap);
	return NULL;
}

KSTATUS schema_sync(void)
{
	KSTATUS _status;
	_status = database_exec("create table if not exists frames "
							"("
							"device text, "
							"type int, "
							"ts text, "
							"len int, "
							"hash text"
							");");
	return _status;
}

double ns_to_s(unsigned long long nanoseconds) {
	double ns = (double)nanoseconds;
	return ns/1.0e9;
}

void printfTimeStat(const char* statsName, stats_key statsKey) {
	printf("%s = %fs\n", statsName, ns_to_s(statsGetValue(statsKey)));
}

int main(int argc, char* argv[])
{
	DPRINTF("main\n");
	KSTATUS _status;
	char *deviceName;

	if(argc < 2) {
		printf("Program usage:\n %s interface-name\n", argv[0]);
		exit(1);
	}
	deviceName = argv[1];
	_status = queuemgr_create(&g_Main._p_framesQueue, 1024*1024);
	_status = svc_kernel_init();
	if(!KSUCCESS(_status))
		goto __exit;
	_status = database_start();
	if(!KSUCCESS(_status))
		goto __exit;
	_status = schema_sync();
	if(!KSUCCESS(_status))
		goto __exit;
	svc_kernel_status(SVC_KERNEL_STATUS_RUNNING);
	g_Main._threads[MAIN_THREAD_PRODUCER]._p_deviceName = deviceName;
	_status = statsAlloc("producerThread", STATS_TYPE_SUM, &g_Main._threads[MAIN_THREAD_PRODUCER]._statsKey);
	if(!KSUCCESS(_status)) {
		printf("Error during allocation StatsKey!\n");
		goto __database_stop_andexit;
	}
	g_Main._threads[MAIN_THREAD_CONSUMER]._p_deviceName = deviceName;
	_status = statsAlloc("consumerTherad", STATS_TYPE_SUM, &g_Main._threads[MAIN_THREAD_CONSUMER]._statsKey);
	if(!KSUCCESS(_status)) {
		printf("Error during allocation StatsKey!\n");
		goto __database_stop_andexit;
	}
	printf("Prepare listen on device=%s\n", argv[1]);
	_status = psmgrCreateThread(pcapproducer_thread_routine, &g_Main._threads[MAIN_THREAD_PRODUCER]);
	if(!KSUCCESS(_status))
		goto __database_stop_andexit;
	_status = psmgrCreateThread(pcapconsumer_thread_routine, &g_Main._threads[MAIN_THREAD_CONSUMER]);
	if(!KSUCCESS(_status))
		goto __database_stop_andexit;
	stats_key statsKeyDBExec = statsFind("db exec time");
	unsigned long long currentTimestamp = timerCurrentTimestamp();
	while(svc_kernel_is_running()) {
		psmgrIdle(1);
		printfTimeStat("producerThread", g_Main._threads[MAIN_THREAD_PRODUCER]._statsKey);
		printfTimeStat("consumerTherad", g_Main._threads[MAIN_THREAD_CONSUMER]._statsKey);
		printfTimeStat("db exec time", statsKeyDBExec);
		printf("Current timestamp = %fs\n", ns_to_s(timerCurrentTimestamp()-currentTimestamp));
	}
	statsFree(g_Main._threads[MAIN_THREAD_PRODUCER]._statsKey);
	statsFree(g_Main._threads[MAIN_THREAD_CONSUMER]._statsKey);
__database_stop_andexit:
	database_stop();
__exit:
	svc_kernel_exit(0);
}
