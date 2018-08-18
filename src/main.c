#include "kernel.h"
#include "svc_kernel/svc_kernel.h"
#include <pcap.h>
#include <unistd.h>
#include <net/ethernet.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <stdbool.h>
#include <signal.h>
#include "svc_kernel/database/database.h"
//Commands
#include "packet_analyze.h"
#include "import_file.h"
#include "export_file.h"

#define MAIN_THREAD_PRODUCER 0
#define MAIN_THREAD_CONSUMER 1

typedef struct _NIUCHACZ_CTX {
	const char* _p_deviceName;
	stats_key _statsKey;
} NIUCHACZ_CTX, *PNIUCHACZ_CTX;

typedef struct _NIUCHACZ_MAIN {
	NIUCHACZ_CTX _threads[2];
	sqlite3 *_db;
} NIUCHACZ_MAIN, *PNIUCHACZ_MAIN;

NIUCHACZ_MAIN g_Main;

static const char * cgCreateSchema =
		"create table if not exists packets ("
		"ts_sec unsigned big int, ts_usec unsigned big int, eth_shost text, eth_dhost text, eth_type int,"
		"ip_vhl int,ip_tos int,ip_len int,ip_id int,ip_off int,ip_ttl int,ip_p int,ip_sum int,ip_src text,ip_dst text"
		");";

sqlite3* getNiuchaczPcapDB() {
	return g_Main._db;
}

pcap_t *gp_PcapHandle;
void pcap_thread_cancelRoutine(void* ptr) {
	pcap_breakloop(gp_PcapHandle);
}
void pcap_thread_ExitRoutine(int signo) {
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

	SYSLOG(LOG_INFO, "PCAP init signals");
	if(signal(SIGTERM, pcap_thread_ExitRoutine) == SIG_ERR)
		return KSTATUS_UNSUCCESS;
	if(signal(SIGINT, pcap_thread_ExitRoutine) == SIG_ERR)
		return KSTATUS_UNSUCCESS;
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
	signal(SIGTERM, SIG_DFL);
	signal(SIGINT, SIG_DFL);
	return KSTATUS_SUCCESS;
}

KSTATUS schema_sync(void)
{
	KSTATUS _status;
	_status = dbExec(getNiuchaczPcapDB(), cgCreateSchema, 0);
	return _status;
}

KSTATUS cmd_sync(void) {
	KSTATUS _status;
	_status = cmdmgrAddCommand("PACKET_ANALYZE", "Analyze network packets and store it in DB file", cmdPacketAnalyzeExec, cmdPacketAnalyzeCreate, cmdPacketAnalyzeDestroy, 1);
	if(!KSUCCESS(_status))
		return _status;
	_status = cmdmgrAddCommand("IMPORT_FILE", "Import PCAP file to DB", cmdImportFileExec, cmdImportFileCreate, cmdImportFileDestroy, 1);
	if(!KSUCCESS(_status))
		return _status;
	_status = cmdmgrAddCommand("EXPORT_FILE", "Export DB to file", cmdExportFileExec, cmdExportFileCreate, cmdExportFileDestroy, 1);
	if(!KSUCCESS(_status))
		return _status;
	return KSTATUS_SUCCESS;
}

KSTATUS testExportFile(const char* file_name) {
    KSTATUS _status;
    PJOB pjob;
    struct timeval ts;
    cmd_export_cfg_t cfg;

    strcpy(cfg._file_name, file_name);
    _status = cmdmgrJobPrepare("EXPORT_FILE", &cfg, sizeof(cmd_export_cfg_t), ts, &pjob);
    if(!KSUCCESS(_status)) {
        SYSLOG(LOG_ERR, "Error during preparing EXPORT_FILE command");
        return _status;
    }
    _status = cmdmgrJobExec(pjob, JobModeSynchronous);
    if(!KSUCCESS(_status)) {
        SYSLOG(LOG_ERR, "Error during processing EXPORT_FILE command");
        return _status;
    }
    return KSTATUS_SUCCESS;
}

int main(int argc, char* argv[])
{
	KSTATUS _status;
	const char *dbFileName, *deviceName;

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
	_status = cmd_sync();
	if(!KSUCCESS(_status))
		goto __database_stop_andexit;
	if(config_lookup(svcKernelGetCfg(), "NIUCHACZ.testMode") == NULL) {
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
	} else {
		if(strcmp(argv[2], "EXPORT_FILE") == 0) {
			testExportFile(argv[3]);
		}
		svcKernelStatus(SVC_KERNEL_STATUS_STOP_PENDING);
	}
	svcKernelMainLoop();
	if(config_lookup(svcKernelGetCfg(), "NIUCHACZ.testMode") == NULL) {
		statsFree(g_Main._threads[MAIN_THREAD_PRODUCER]._statsKey);
		statsFree(g_Main._threads[MAIN_THREAD_CONSUMER]._statsKey);
	}
__database_stop_andexit:
	dbStop(getNiuchaczPcapDB());
__exit:
	svcKernelExit(0);
}
