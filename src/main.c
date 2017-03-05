#define _GNU_SOURCE
#include "kernel.h"
#include "psmgr/psmgr.h"
#include "database/database.h"
#include <openssl/md5.h>
#include <pcap.h>
#include <unistd.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include "queuemgr/queuemgr.h"
////////////////////
#include <time.h>

struct timespec timer_start(){
    struct timespec start_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);
    return start_time;
}

long timer_end(struct timespec start_time){
    struct timespec end_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time);
    long diffInNanos = end_time.tv_nsec - start_time.tv_nsec;
    return diffInNanos;
}
////////////////////

PQUEUE gpFramesQueue;

void framesproducer_callback(const char* device, unsigned char *packet, struct timeval ts, unsigned int packet_len) {
	KSTATUS _status;

	struct timespec vartime = timer_start();
	_status = queuemgr_enqueue(gpFramesQueue, ts, packet, packet_len);
	if(!KSUCCESS(_status))
	{
		printf("Error in enqueue msg!\n");
		return;
	}
    long time_elapsed_nanos = timer_end(vartime);
    printf("[Producer]Time taken (nanoseconds): %ld\n", time_elapsed_nanos);
}

void framesconsumer_callback(const char* device, unsigned char *buffer) {
	MD5_CTX c;
	unsigned char packet_md5[MD5_DIGEST_LENGTH];
	char stmt[512];
	struct ether_header *eth_header;
	size_t packet_len;
	struct timeval timestamp;
	KSTATUS _status;

	struct timespec vartime = timer_start();
	_status = queuemgr_dequeue(gpFramesQueue, &timestamp, buffer, &packet_len);
	if(!KSUCCESS(_status))
	{
		printf("Error in dequeue msg!\n");
		return;
	}
	MD5_Init(&c);
    MD5_Update(&c, buffer, packet_len);
    MD5_Final((unsigned char*)packet_md5, &c);
    eth_header = (struct ether_header *) buffer;
    sprintf(stmt, "insert into frames (device, src_mac, dst_mac, type, ts, len, hash) "
    			  "values (\"%s\", %u, %d.%06d, %zu, \"%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\")",
				  device,
				  ntohs(eth_header->ether_type),
				  (int) timestamp.tv_sec, (int) timestamp.tv_usec, packet_len,
				  packet_md5[0], packet_md5[1], packet_md5[2], packet_md5[3],
				  packet_md5[4], packet_md5[5], packet_md5[6], packet_md5[7],
				  packet_md5[8], packet_md5[9], packet_md5[10], packet_md5[11],
				  packet_md5[12], packet_md5[13], packet_md5[14], packet_md5[15]);
    database_exec(stmt);
    long time_elapsed_nanos = timer_end(vartime);
    printf("[Consumer]Time taken (nanoseconds): %ld\n", time_elapsed_nanos);
}

void* pcapconsumer_thread_routine(void* arg)
{
	const char* device = (const char*)arg;
	unsigned char *buffer = MALLOC(unsigned char, 1024*1024);

	while(svc_kernel_is_running()) {
		framesconsumer_callback(device, buffer);
	}
	return NULL;
}

void* pcapproducer_thread_routine(void* arg)
{
	const char* device = (const char*)arg;
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_t *pcap;
	struct pcap_pkthdr header;
	unsigned char *packet;

    printf("Listen on device=%s\n", device);
	pcap = pcap_open_live(device, BUFSIZ, 0, 1000, errbuf);
	if(pcap == NULL) {
		fprintf(stderr, "error reading pcap file: %s\n", errbuf);
		return NULL;
	}
	while(svc_kernel_is_running()) {
		packet = (unsigned char*)pcap_next(pcap, &header);
		if(packet != NULL)
		{
			framesproducer_callback(device, packet, header.ts, header.caplen);
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

int main(int argc, char* argv[])
{
	DPRINTF("main\n");
	KSTATUS _status;

	_status = queuemgr_create(&gpFramesQueue, 1024*1024);
	_status = svc_kernel_init();
	if(!KSUCCESS(_status))
		goto __exit;
	_status = psmgr_start();
	if(!KSUCCESS(_status))
		goto __exit;
	_status = database_start();
	if(!KSUCCESS(_status))
		goto __psmgr_stop_andexit;
	_status = schema_sync();
	if(!KSUCCESS(_status))
		goto __psmgr_stop_andexit;
	svc_kernel_status(SVC_KERNEL_STATUS_RUNNING);

	struct ifaddrs *ifaddr, *ifa;
	int n;
	if(getifaddrs(&ifaddr) == -1)
	{
		perror("getifaddrs");
		goto __database_stop_andexit;
	}

	for(ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++)
	{
		if(ifa->ifa_addr == NULL)
		{
		    printf("Prepare listen on device=NULL\n");
			continue;
		}
		printf("%-8s %s (%d)\n", ifa->ifa_name,
		                      (ifa->ifa_addr->sa_family == AF_PACKET) ? "AF_PACKET" :
		                      (ifa->ifa_addr->sa_family == AF_INET) ? "AF_INET" :
		                      (ifa->ifa_addr->sa_family == AF_INET6) ? "AF_INET6" : "???",
		                      ifa->ifa_addr->sa_family);
		if(ifa->ifa_addr->sa_family != AF_PACKET)
			continue;
	    printf("Prepare listen on device=%s\n", ifa->ifa_name);
		_status = psmgr_create_thread(pcapproducer_thread_routine, ifa->ifa_name);
		if(!KSUCCESS(_status))
			goto __freeaddr_database_stop_andexit;
		_status = psmgr_create_thread(pcapconsumer_thread_routine, ifa->ifa_name);
		if(!KSUCCESS(_status))
			goto __freeaddr_database_stop_andexit;
	}
	psmgr_idle();
__freeaddr_database_stop_andexit:
	freeifaddrs(ifaddr);
__database_stop_andexit:
	database_stop();
__psmgr_stop_andexit:
	psmgr_stop();
__exit:
	svc_kernel_exit(0);
}
