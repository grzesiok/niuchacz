#include "kernel.h"
#include "psmgr/psmgr.h"
#include <openssl/md5.h>
#include <pcap.h>
#include <signal.h>
#include <unistd.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include <sys/types.h>
#include <ifaddrs.h>

bool isRunning;
pthread_mutex_t	mutex = PTHREAD_MUTEX_INITIALIZER;

void frames_callback(const char* device, const unsigned char *packet, struct timeval ts, unsigned int packet_len) {
	MD5_CTX c;
	unsigned char packet_md5[MD5_DIGEST_LENGTH];
	unsigned char stmt[512];
	struct ether_header *eth_header;

	MD5_Init(&c);
    MD5_Update(&c, packet, packet_len);
    MD5_Final(packet_md5, &c);
    eth_header = (struct ether_header *) packet;
    sprintf(stmt, "insert into frames (device, type, ts, len, hash) "
    			  "values (\"%s\", %u, %d.%06d, %d, \"%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\")",
				  device,
				  ntohs(eth_header->ether_type),
				  (int) ts.tv_sec, (int) ts.tv_usec, packet_len,
				  packet_md5[0], packet_md5[1], packet_md5[2], packet_md5[3],
				  packet_md5[4], packet_md5[5], packet_md5[6], packet_md5[7],
				  packet_md5[8], packet_md5[9], packet_md5[10], packet_md5[11],
				  packet_md5[12], packet_md5[13], packet_md5[14], packet_md5[15]);
    pthread_mutex_lock(&mutex);
    printf("%s\n", stmt);//sql_stmt(stmt);
    pthread_mutex_unlock(&mutex);
}

void* pcap_thread_routine(void* arg)
{
	const char* device = (const char*)arg;
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_t *pcap;
	struct pcap_pkthdr header;
	const unsigned char *packet;
	int i;

    pthread_mutex_lock(&mutex);
    printf("Listen on device=%s\n", device);
    pthread_mutex_unlock(&mutex);
	pcap = pcap_open_live(device, BUFSIZ, 0, 1000, errbuf);
	if(pcap == NULL) {
		fprintf(stderr, "error reading pcap file: %s\n", errbuf);
		return -1;
	}
	while(isRunning) {
		packet = pcap_next(pcap, &header);
		if(packet != NULL)
		{
			frames_callback(device, packet, header.ts, header.caplen);
		}
	}
	pcap_close(pcap);
	return 0;
}

void sig_handler(int signo)
{
	if(signo == SIGINT)
		isRunning = false;
}

int main(int argc, char* argv[])
{
	DPRINTF("main\n");
	KSTATUS _status;

	isRunning = true;
	if(signal(SIGINT, sig_handler) == SIG_ERR)
		perror("\ncan't catch SIGINT\n");
	_status = psmgr_start();
	if(!KSUCCESS(_status))
		goto __exit;
	_status = database_start();
	if(!KSUCCESS(_status))
		goto __psmgr_stop_andexit;

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
	    printf("Prepare listen on device=%s\n", ifa->ifa_name);
		_status = psmgr_create_thread(pcap_thread_routine, ifa->ifa_name);
	}
	psmgr_idle();
	freeifaddrs(ifaddr);
__database_stop_andexit:
	database_stop();
__psmgr_stop_andexit:
	psmgr_stop();
__exit:
	return 0;
}
