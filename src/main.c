#include "kernel.h"
#include "svc_kernel/svc_kernel.h"
#include "database/database.h"
#include <pcap.h>
#include <unistd.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include "mapper.h"
#include <arpa/inet.h>
#include <stdbool.h>
#include <netinet/ether.h>

#define MAIN_THREAD_PRODUCER 0
#define MAIN_THREAD_CONSUMER 1

typedef struct _NIUCHACZ_CTX {
	char* _p_deviceName;
	stats_key _statsKey;
} NIUCHACZ_CTX, *PNIUCHACZ_CTX;

typedef struct _NIUCHACZ_MAIN {
	NIUCHACZ_CTX _threads[2];
    sqlite3_stmt *_stmt;
} NIUCHACZ_MAIN, *PNIUCHACZ_MAIN;

NIUCHACZ_MAIN g_Main;

const char * cgCreateSchema =
		"create table if not exists packets ("
		"ts_sec unsigned big int, ts_usec unsigned big int, eth_shost text, eth_dhost text, eth_type int,"
		"ip_vhl int,ip_tos int,ip_len int,ip_id int,ip_off int,ip_ttl int,ip_p int,ip_sum int,ip_src text,ip_dst text"
		");";
const char * cgStmt =
		"insert into packets(ts_sec, ts_usec,eth_shost,eth_dhost,eth_type,"
		"ip_vhl,ip_tos,ip_len,ip_id,ip_off,ip_ttl,ip_p,ip_sum,ip_src,ip_dst)"
		"values (?,?,?,?,?,"
		"?,?,?,?,?,?,?,?,?,?);";

void frames_callback(const char* device, unsigned char *packet, struct timeval ts, unsigned int packet_len) {
	MAPPER_RESULTS results;
	struct timespec startTime;

    if(!mapFrame(packet, packet_len, &results)) {
    	fprintf(stderr, "Error in parsing message!\n");
		return;
	}
    //ts_sec since Epoch (1970) timestamp						  /* Timestamp of packet */
    if(!database_bind_int64(true, g_Main._stmt, 1, ts.tv_sec)) {
        return;
    }//ts_usec /* Microseconds */
    if(!database_bind_int64(true, g_Main._stmt, 2, ts.tv_usec)) {
        return;
    }
    //u_char  eth_shost[ETHER_ADDR_LEN];      /* source host address */
    if(!database_bind_text(true, g_Main._stmt, 3, ether_ntoa((struct ether_addr*)&results._ethernet.eth_shost)) != SQLITE_OK) {
        return;
    }
    //u_char  eth_dhost[ETHER_ADDR_LEN];      /* destination host address */
    if(!database_bind_text(true, g_Main._stmt, 4, ether_ntoa((struct ether_addr*)&results._ethernet.eth_dhost)) != SQLITE_OK) {
        return;
    }
    //u_short eth_type;                       /* IP? ARP? RARP? etc */
    if(!database_bind_int(true, g_Main._stmt, 5, results._ethernet.eth_type) != SQLITE_OK) {
        return;
    }
    //u_char  ip_vhl;                 		  /* version << 4 | header length >> 2 */
    if(!database_bind_int(true, g_Main._stmt, 6, results._ip.ip_vhl) != SQLITE_OK) {
        return;
    }
    //u_char  ip_tos;                 		  /* type of service */
    if(!database_bind_int(true, g_Main._stmt, 7, results._ip.ip_tos) != SQLITE_OK) {
        return;
    }
    //u_short ip_len;                 		  /* total length */
    if(!database_bind_int(true, g_Main._stmt, 8, results._ip.ip_len) != SQLITE_OK) {
        return;
    }
    //u_short ip_id;                  		  /* identification */
    if(!database_bind_int(true, g_Main._stmt, 9, results._ip.ip_id) != SQLITE_OK) {
        return;
    }
    //u_short ip_off;                 		  /* fragment offset field */
    if(!database_bind_int(true, g_Main._stmt, 10, results._ip.ip_off) != SQLITE_OK) {
        return;
    }
    //u_char  ip_ttl;                 		  /* time to live */
    if(!database_bind_int(true, g_Main._stmt, 11, results._ip.ip_ttl) != SQLITE_OK) {
        return;
    }
    //u_char  ip_p;                   		  /* protocol */
    if(!database_bind_int(true, g_Main._stmt, 12, results._ip.ip_p) != SQLITE_OK) {
        return;
    }
    //u_short ip_sum;                 		  /* checksum */
    if(!database_bind_int(true, g_Main._stmt, 13, results._ip.ip_sum) != SQLITE_OK) {
        return;
    }
    //struct  in_addr ip_src;  		  /* source address */
    if(!database_bind_text(true, g_Main._stmt, 14, inet_ntoa(results._ip.ip_src)) != SQLITE_OK) {
        return;
    }
    //struct  in_addr ip_dst;  		  /* dest address */
    if(!database_bind_text(true, g_Main._stmt, 15, inet_ntoa(results._ip.ip_dst)) != SQLITE_OK) {
        return;
    }
	startTime = timerStart();
    if(sqlite3_step(g_Main._stmt) != SQLITE_DONE) {
    	fprintf(stderr, "%s\n", database_errmsg());
    }
    sqlite3_reset(g_Main._stmt);
	statsUpdate(g_statsKey_DbExecTime, timerStop(startTime));
}

void* pcap_thread_routine(void* arg)
{
	PNIUCHACZ_CTX p_ctx = (PNIUCHACZ_CTX)arg;
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_t *handle;
	struct pcap_pkthdr header;
	unsigned char *packet;
	struct timespec startTime;
	char filter_exp[] = "ip"; /* filter expression (only IP packets) */
	struct bpf_program fp; /* compiled filter program (expression) */
	bpf_u_int32 net;
	bpf_u_int32 mask;

    printf("Listen on device=%s\n", p_ctx->_p_deviceName);
    /* get network number and mask associated with capture device */
    if (pcap_lookupnet(p_ctx->_p_deviceName, &net, &mask, errbuf) == -1) {
    	fprintf(stderr, "Couldn't get netmask for device %s: %s\n", p_ctx->_p_deviceName, errbuf);
    	net = 0;
    	mask = 0;
    }
    /* open capture device */
    handle = pcap_open_live(p_ctx->_p_deviceName, BUFSIZ, 0, 1000, errbuf);
	if(handle == NULL) {
		fprintf(stderr, "Couldn't open device %s: %s\n", p_ctx->_p_deviceName, errbuf);
		return NULL;
	}
	/* make sure we're capturing on an Ethernet device */
	if(pcap_datalink(handle) != DLT_EN10MB) {
		fprintf(stderr, "%s is not an Ethernet\n", p_ctx->_p_deviceName);
		exit(EXIT_FAILURE);
	}
	/* compile the filter expression */
	if(pcap_compile(handle, &fp, filter_exp, 0, net) == -1) {
		fprintf(stderr, "Couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(handle));
		exit(EXIT_FAILURE);
	}
	/* apply the compiled filter */
	if (pcap_setfilter(handle, &fp) == -1) {
		fprintf(stderr, "Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(handle));
		exit(EXIT_FAILURE);
	}
	while(svc_kernel_is_running()) {
		packet = (unsigned char*)pcap_next(handle, &header);
		if(packet != NULL)
		{
			startTime = timerStart();
			frames_callback(p_ctx->_p_deviceName, packet, header.ts, header.caplen);
			statsUpdate(p_ctx->_statsKey, timerStop(startTime));
		}
	}
	/* cleanup */
	pcap_freecode(&fp);
	pcap_close(handle);
	return NULL;
}

KSTATUS schema_sync(void)
{
	KSTATUS _status;
	_status = database_exec(cgCreateSchema);
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

	if(argc < 3) {
		printf("Program usage:\n %s interface-name db-path\n", argv[0]);
		exit(1);
	}
	deviceName = argv[1];
	_status = svc_kernel_init();
	if(!KSUCCESS(_status))
		goto __exit;
	_status = database_start(argv[2]);
	if(!KSUCCESS(_status))
		goto __exit;
	_status = schema_sync();
	if(!KSUCCESS(_status))
		goto __exit;
	svc_kernel_status(SVC_KERNEL_STATUS_RUNNING);
    if(sqlite3_prepare(database_getinstance(), cgStmt, -1, &g_Main._stmt, 0) != SQLITE_OK) {
        printf("\nCould not prepare statement.");
		goto __database_stop_andexit;
    }
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
	_status = psmgrCreateThread(pcap_thread_routine, &g_Main._threads[MAIN_THREAD_PRODUCER]);
	if(!KSUCCESS(_status))
		goto __database_stop_andexit;
	while(svc_kernel_is_running()) {
		psmgrIdle(1);
	}
    sqlite3_finalize(g_Main._stmt);
	statsFree(g_Main._threads[MAIN_THREAD_PRODUCER]._statsKey);
	statsFree(g_Main._threads[MAIN_THREAD_CONSUMER]._statsKey);
__database_stop_andexit:
	database_stop();
__exit:
	svc_kernel_exit(0);
}
