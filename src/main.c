#include "kernel.h"
#include "svc_kernel/svc_kernel.h"
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
#include "svc_kernel/database/database.h"

#define MAIN_THREAD_PRODUCER 0
#define MAIN_THREAD_CONSUMER 1

typedef struct _NIUCHACZ_CTX {
	const char* _p_deviceName;
	stats_key _statsKey;
} NIUCHACZ_CTX, *PNIUCHACZ_CTX;

typedef struct _NIUCHACZ_MAIN {
	NIUCHACZ_CTX _threads[2];
	sqlite3_stmt *_stmt;
	sqlite3 *_db;
} NIUCHACZ_MAIN, *PNIUCHACZ_MAIN;

NIUCHACZ_MAIN g_Main;

static const char * cgCreateSchema =
		"create table if not exists packets ("
		"ts_sec unsigned big int, ts_usec unsigned big int, eth_shost text, eth_dhost text, eth_type int,"
		"ip_vhl int,ip_tos int,ip_len int,ip_id int,ip_off int,ip_ttl int,ip_p int,ip_sum int,ip_src text,ip_dst text"
		");";
static const char * cgStmt =
		"insert into packets(ts_sec, ts_usec,eth_shost,eth_dhost,eth_type,"
		"ip_vhl,ip_tos,ip_len,ip_id,ip_off,ip_ttl,ip_p,ip_sum,ip_src,ip_dst)"
		"values (?,?,?,?,?,"
		"?,?,?,?,?,?,?,?,?,?);";

int packet_analyze(struct timeval ts, void* packet, size_t packet_len) {
	MAPPER_RESULTS results;
	struct timespec startTime;

    if(!mapFrame((unsigned char *)packet, packet_len, &results)) {
    	SYSLOG(LOG_ERR, "Error in parsing message!");
		return -1;
	}
    //ts_sec since Epoch (1970) timestamp						  /* Timestamp of packet */
    if(!dbBind_int64(true, g_Main._stmt, 1, ts.tv_sec)) {
        return -1;
    }//ts_usec /* Microseconds */
    if(!dbBind_int64(true, g_Main._stmt, 2, ts.tv_usec)) {
        return -1;
    }
    //u_char  eth_shost[ETHER_ADDR_LEN];      /* source host address */
    if(!dbBind_text(true, g_Main._stmt, 3, ether_ntoa((struct ether_addr*)&results._ethernet.eth_shost)) != SQLITE_OK) {
        return -1;
    }
    //u_char  eth_dhost[ETHER_ADDR_LEN];      /* destination host address */
    if(!dbBind_text(true, g_Main._stmt, 4, ether_ntoa((struct ether_addr*)&results._ethernet.eth_dhost)) != SQLITE_OK) {
        return -1;
    }
    //u_short eth_type;                       /* IP? ARP? RARP? etc */
    if(!dbBind_int(true, g_Main._stmt, 5, results._ethernet.eth_type) != SQLITE_OK) {
        return -1;
    }
    //u_char  ip_vhl;                 		  /* version << 4 | header length >> 2 */
    if(!dbBind_int(true, g_Main._stmt, 6, results._ip.ip_vhl) != SQLITE_OK) {
        return -1;
    }
    //u_char  ip_tos;                 		  /* type of service */
    if(!dbBind_int(true, g_Main._stmt, 7, results._ip.ip_tos) != SQLITE_OK) {
        return -1;
    }
    //u_short ip_len;                 		  /* total length */
    if(!dbBind_int(true, g_Main._stmt, 8, results._ip.ip_len) != SQLITE_OK) {
        return -1;
    }
    //u_short ip_id;                  		  /* identification */
    if(!dbBind_int(true, g_Main._stmt, 9, results._ip.ip_id) != SQLITE_OK) {
        return -1;
    }
    //u_short ip_off;                 		  /* fragment offset field */
    if(!dbBind_int(true, g_Main._stmt, 10, results._ip.ip_off) != SQLITE_OK) {
        return -1;
    }
    //u_char  ip_ttl;                 		  /* time to live */
    if(!dbBind_int(true, g_Main._stmt, 11, results._ip.ip_ttl) != SQLITE_OK) {
        return -1;
    }
    //u_char  ip_p;                   		  /* protocol */
    if(!dbBind_int(true, g_Main._stmt, 12, results._ip.ip_p) != SQLITE_OK) {
        return -1;
    }
    //u_short ip_sum;                 		  /* checksum */
    if(!dbBind_int(true, g_Main._stmt, 13, results._ip.ip_sum) != SQLITE_OK) {
        return -1;
    }
    //struct  in_addr ip_src;  		  /* source address */
    if(!dbBind_text(true, g_Main._stmt, 14, inet_ntoa(results._ip.ip_src)) != SQLITE_OK) {
        return -1;
    }
    //struct  in_addr ip_dst;  		  /* dest address */
    if(!dbBind_text(true, g_Main._stmt, 15, inet_ntoa(results._ip.ip_dst)) != SQLITE_OK) {
        return -1;
    }
	startTime = timerStart();
    if(sqlite3_step(g_Main._stmt) != SQLITE_DONE) {
    	SYSLOG(LOG_ERR, "%s", dbGetErrmsg(g_Main._db));
    }
    sqlite3_reset(g_Main._stmt);
	statsUpdate(g_statsKey_DbExecTime, timerStop(startTime));
	return 0;
}

pcap_t *gp_PcapHandle;
void pcap_thread_cancelRoutine(void* arg) {
	pcap_breakloop(gp_PcapHandle);
}

KSTATUS pcap_thread_routine(void* arg)
{
	PNIUCHACZ_CTX p_ctx = (PNIUCHACZ_CTX)arg;
	char errbuf[PCAP_ERRBUF_SIZE];
	struct pcap_pkthdr header;
	void* packet;
	struct timespec startTime;
	char filter_exp[] = "ip"; /* filter expression (only IP packets) */
	struct bpf_program fp; /* compiled filter program (expression) */
	bpf_u_int32 net;
	bpf_u_int32 mask;
	KSTATUS _status;
	PJOB pjob;

	SYSLOG(LOG_INFO, "Listen on device=%s", p_ctx->_p_deviceName);
	/* get network number and mask associated with capture device */
	if (pcap_lookupnet(p_ctx->_p_deviceName, &net, &mask, errbuf) == -1) {
		SYSLOG(LOG_ERR, "Couldn't get netmask for device %s: %s", p_ctx->_p_deviceName, errbuf);
		net = 0;
		mask = 0;
	}
	/* open capture device */
	gp_PcapHandle = pcap_open_live(p_ctx->_p_deviceName, BUFSIZ, 0, 1000, errbuf);
	if(gp_PcapHandle == NULL) {
		SYSLOG(LOG_ERR, "Couldn't open device %s: %s", p_ctx->_p_deviceName, errbuf);
		return KSTATUS_UNSUCCESS;
	}
	/* make sure we're capturing on an Ethernet device */
	if(pcap_datalink(gp_PcapHandle) != DLT_EN10MB) {
		SYSLOG(LOG_ERR, "%s is not an Ethernet", p_ctx->_p_deviceName);
		return KSTATUS_UNSUCCESS;
	}
	/* compile the filter expression */
	if(pcap_compile(gp_PcapHandle, &fp, filter_exp, 0, net) == -1) {
		SYSLOG(LOG_ERR, "Couldn't parse filter %s: %s", filter_exp, pcap_geterr(gp_PcapHandle));
		return KSTATUS_UNSUCCESS;
	}
	/* apply the compiled filter */
	if (pcap_setfilter(gp_PcapHandle, &fp) == -1) {
		SYSLOG(LOG_ERR, "Couldn't install filter %s: %s", filter_exp, pcap_geterr(gp_PcapHandle));
		return KSTATUS_UNSUCCESS;
	}
	while(svcKernelIsRunning()) {
		packet = (void*)pcap_next(gp_PcapHandle, &header);
		if(packet != NULL) {
			startTime = timerStart();
			_status = cmdmgrJobPrepare("PACKET_ANALYZE", packet, header.caplen, header.ts, &pjob);
			if(!KSUCCESS(_status)) {
				SYSLOG(LOG_ERR, "Couldn't prepare packet to analyze");
				continue;
			}
			_status = cmdmgrJobExec(pjob, JobModeAsynchronous);
			if(!KSUCCESS(_status)) {
				SYSLOG(LOG_ERR, "Couldn't execute job");
				continue;
			}
			statsUpdate(p_ctx->_statsKey, timerStop(startTime));
		}
	}
	/* cleanup */
	pcap_freecode(&fp);
	pcap_close(gp_PcapHandle);
	return KSTATUS_SUCCESS;
}

KSTATUS schema_sync(void)
{
	KSTATUS _status;
	_status = dbExec(g_Main._db, cgCreateSchema);
	return _status;
}

int main(int argc, char* argv[])
{
	KSTATUS _status;
	const char *dbFileName, *deviceName;

	if(argc != 2)
		return -1;
	_status = svcKernelInit(argv[1]);
	if(!KSUCCESS(_status))
		goto __exit;
	if(!config_lookup_string(svcKernelGetCfg(), "DB.fileName", &dbFileName)) {
		SYSLOG(LOG_ERR, "%s:%d - %s\n", config_error_file(svcKernelGetCfg()), config_error_line(svcKernelGetCfg()), config_error_text(svcKernelGetCfg()));
		goto __exit;
	}
	if(!config_lookup_string(svcKernelGetCfg(), "NIUCHACZ.deviceName", &deviceName)) {
		SYSLOG(LOG_ERR, "%s:%d - %s\n", config_error_file(svcKernelGetCfg()), config_error_line(svcKernelGetCfg()), config_error_text(svcKernelGetCfg()));
		goto __exit;
	}
	_status = dbStart(dbFileName, &g_Main._db);
	if(!KSUCCESS(_status))
		goto __exit;
	_status = schema_sync();
	if(!KSUCCESS(_status))
		goto __database_stop_andexit;
	svcKernelStatus(SVC_KERNEL_STATUS_RUNNING);
	_status = cmdmgrAddCommand("PACKET_ANALYZE", "Analyze network packets and store it in DB file", packet_analyze, 1);
	if(!KSUCCESS(_status))
		goto __database_stop_andexit;
	if(sqlite3_prepare(g_Main._db, cgStmt, -1, &g_Main._stmt, 0) != SQLITE_OK) {
		SYSLOG(LOG_ERR, "Could not prepare statement.");
		goto __database_stop_andexit;
	}
	g_Main._threads[MAIN_THREAD_PRODUCER]._p_deviceName = deviceName;
	_status = statsAlloc("producerThread", STATS_TYPE_SUM, &g_Main._threads[MAIN_THREAD_PRODUCER]._statsKey);
	if(!KSUCCESS(_status)) {
		SYSLOG(LOG_ERR, "Error during allocation StatsKey!");
		goto __database_stop_andexit;
	}
	g_Main._threads[MAIN_THREAD_CONSUMER]._p_deviceName = deviceName;
	_status = statsAlloc("consumerThread", STATS_TYPE_SUM, &g_Main._threads[MAIN_THREAD_CONSUMER]._statsKey);
	if(!KSUCCESS(_status)) {
		SYSLOG(LOG_ERR, "Error during allocation StatsKey!");
		goto __database_stop_andexit;
	}
	SYSLOG(LOG_INFO, "Prepare listen on device=%s", deviceName);
	_status = psmgrCreateThread("pcapThreadRoutine", PSMGR_THREAD_USER, pcap_thread_routine, pcap_thread_cancelRoutine, &g_Main._threads[MAIN_THREAD_PRODUCER]);
	if(!KSUCCESS(_status))
		goto __database_stop_andexit;
	svcKernelMainLoop();
	sqlite3_finalize(g_Main._stmt);
	statsFree(g_Main._threads[MAIN_THREAD_PRODUCER]._statsKey);
	statsFree(g_Main._threads[MAIN_THREAD_CONSUMER]._statsKey);
__database_stop_andexit:
	dbStop(g_Main._db);
__exit:
	svcKernelExit(0);
}
