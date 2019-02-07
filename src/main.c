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
#include "algorithms.h"
//Commands
#include "packet_analyze.h"
#include "import_file.h"
#include "export_file.h"

typedef struct _NIUCHACZ_MAIN {
    const char* _p_deviceName;
    database_t* _db;
} NIUCHACZ_MAIN, *PNIUCHACZ_MAIN;

NIUCHACZ_MAIN g_Main;

static const char * cgCreateSchemaPackets =
		"create table if not exists packets("
		    "ts_sec unsigned big int,"
                    "ts_usec unsigned big int,"
                    "eth_src_id int,"
                    "eth_dst_id int,"
                    "eth_type int,"
		    "ip_vhl int,"
                    "ip_tos int,"
                    "ip_len int,"
                    "ip_id int,"
                    "ip_off int,"
                    "ip_ttl int,"
                    "ip_p int,"
                    "ip_sum int,"
                    "ip_src_id int,"
                    "ip_dst_id int,"
                    "payload text,"
                    "foreign key(eth_src_id) references eth(eth_id),"
                    "foreign key(eth_dst_id) references eth(eth_id),"
                    "foreign key(ip_src_id) references ip(ip_id),"
                    "foreign key(ip_dst_id) references ip(ip_id)"
		");";
static const char * cgCreateSchemaEth =
                "create table if not exists eth("
                    "eth_id integer primary key,"
                    "ts_sec unsigned big int,"
                    "ts_usec unsigned int,"
                    "eth_addr text,"
                    "activeflag int"
                ");";
static const char * cgCreateSchemaIP =
                "create table if not exists ip("
                    "ip_id integer primary key,"
                    "ts_sec unsigned big int,"
                    "ts_usec unsigned int,"
                    "ip_addr text,"
                    "hostname text,"
                    "activeflag int"
                ");";
static const char * cgCreateView_PacketsPerDate =
                "create view if not exists report$packetsperdate"
                " as "
                "select report_date, count(*) cnt, sum(ip_len) as bytes from (select date(datetime(ts_sec, 'unixepoch')) as report_date, ip_len from packets) group by report_date order by report_date desc;";

database_t* getNiuchaczPcapDB() {
    return g_Main._db;
}

pcap_t *gp_PcapHandle;
void pcap_thread_cancelRoutine(void* ptr) {
    printf("pcap_thread_cancelRoutine\n");
    pcap_breakloop(gp_PcapHandle);
}
void pcap_thread_ExitRoutine(int signo) {
    printf("pcap_thread_ExitRoutine\n");
    pcap_breakloop(gp_PcapHandle);
}

KSTATUS pcap_thread_routine(void* arg)
{
    char errbuf[PCAP_ERRBUF_SIZE];
    struct pcap_pkthdr header;
    void* packet;
    char filter_exp[] = "ip"; /* filter expression (only IP packets) */
    struct bpf_program fp; /* compiled filter program (expression) */
    bpf_u_int32 net;
    bpf_u_int32 mask;
    KSTATUS _status;
    PJOB pjob;
    const char* p_deviceName = g_Main._p_deviceName;

    SYSLOG(LOG_INFO, "Listen on device=%s", p_deviceName);
    /* get network number and mask associated with capture device */
    if(pcap_lookupnet(p_deviceName, &net, &mask, errbuf) == -1) {
        SYSLOG(LOG_ERR, "Couldn't get netmask for device %s: %s", p_deviceName, errbuf);
        net = 0;
        mask = 0;
    }
    /* open capture device */
    gp_PcapHandle = pcap_open_live(p_deviceName, BUFSIZ, 0, 1000, errbuf);
    if(gp_PcapHandle == NULL) {
        SYSLOG(LOG_ERR, "Couldn't open device %s: %s", p_deviceName, errbuf);
        return KSTATUS_UNSUCCESS;
    }
    /* make sure we're capturing on an Ethernet device */
    if(pcap_datalink(gp_PcapHandle) != DLT_EN10MB) {
        SYSLOG(LOG_ERR, "%s is not an Ethernet", p_deviceName);
        return KSTATUS_UNSUCCESS;
    }
    /* compile the filter expression */
    if(pcap_compile(gp_PcapHandle, &fp, filter_exp, 0, net) == -1) {
        SYSLOG(LOG_ERR, "Couldn't parse filter %s: %s", filter_exp, pcap_geterr(gp_PcapHandle));
        return KSTATUS_UNSUCCESS;
    }
    /* apply the compiled filter */
    if(pcap_setfilter(gp_PcapHandle, &fp) == -1) {
        SYSLOG(LOG_ERR, "Couldn't install filter %s: %s", filter_exp, pcap_geterr(gp_PcapHandle));
        return KSTATUS_UNSUCCESS;
    }
    while(svcKernelIsRunning()) {
        packet = (void*)pcap_next(gp_PcapHandle, &header);
        if(packet != NULL) {
            _status = cmdmgrJobPrepare("PACKET_ANALYZE", packet, header.caplen, header.ts, &pjob);
            if(!KSUCCESS(_status)) {
                SYSLOG(LOG_ERR, "Couldn't prepare packet to analyze");
                continue;
            }
            _status = cmdmgrJobExec(pjob, JobModeAsynchronous, JobQueueTypeShortOps);
            if(!KSUCCESS(_status)) {
                SYSLOG(LOG_ERR, "Couldn't execute job");
                cmdmgrJobCleanup(pjob);
                continue;
            }
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
    SYSLOG(LOG_INFO, "SYNC DB START");
    _status = dbExec(getNiuchaczPcapDB(), cgCreateSchemaEth, 0);
    if(!KSUCCESS(_status))
        return _status;
    _status = dbExec(getNiuchaczPcapDB(), cgCreateSchemaIP, 0);
    if(!KSUCCESS(_status))
        return _status;
    _status = dbExec(getNiuchaczPcapDB(), cgCreateSchemaPackets, 0);
    if(!KSUCCESS(_status))
        return _status;
    _status = dbExec(getNiuchaczPcapDB(), cgCreateView_PacketsPerDate, 0);
    SYSLOG(LOG_INFO, "SYNC DB STOP");
    return _status;
}

KSTATUS cmd_sync(void) {
	KSTATUS _status;
	_status = cmdmgrAddCommand("PACKET_ANALYZE", "Analyze network packets and store it in DB file", cmdPacketAnalyzeExec, cmdPacketAnalyzeCreate, cmdPacketAnalyzeDestroy, 1);
	if(!KSUCCESS(_status))
		return _status;
	_status = cmdmgrAddCommand("IMPORT_PCAP", "Import PCAP file to DB", cmdImportFileExec, cmdImportFileCreate, cmdImportFileDestroy, 1);
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

    SYSLOG(LOG_INFO, "Exporting file=%s", file_name);
    strcpy(cfg._file_name, file_name);
    _status = cmdmgrJobPrepare("EXPORT_FILE", &cfg, sizeof(cmd_export_cfg_t), ts, &pjob);
    if(!KSUCCESS(_status)) {
        SYSLOG(LOG_ERR, "Error during preparing EXPORT_FILE command");
        return _status;
    }
    _status = cmdmgrJobExec(pjob, JobModeSynchronous, JobQueueTypeNone);
    if(!KSUCCESS(_status)) {
        SYSLOG(LOG_ERR, "Error during processing EXPORT_FILE command");
        return _status;
    }
    return KSTATUS_SUCCESS;
}

KSTATUS testImportPcap(const char* file_name) {
    KSTATUS _status;
    PJOB pjob;
    struct timeval ts;
    cmd_export_cfg_t cfg;

    SYSLOG(LOG_INFO, "Importing PCAP file=%s", file_name);
    strcpy(cfg._file_name, file_name);
    _status = cmdmgrJobPrepare("IMPORT_PCAP", &cfg, sizeof(cmd_export_cfg_t), ts, &pjob);
    if(!KSUCCESS(_status)) {
        SYSLOG(LOG_ERR, "Error during preparing IMPORT_PCAP command");
        return _status;
    }
    _status = cmdmgrJobExec(pjob, JobModeSynchronous, JobQueueTypeNone);
    if(!KSUCCESS(_status)) {
        SYSLOG(LOG_ERR, "Error during processing IMPORT_PCAP command");
        return _status;
    }
    return KSTATUS_SUCCESS;
}

int main(int argc, char* argv[])
{
	KSTATUS _status;
	const char *dbFileName;

	_status = svcKernelInit(argv[1]);
	if(!KSUCCESS(_status))
		goto __exit;
	if(!config_lookup_string(svcKernelGetCfg(), "DB.fileName", &dbFileName)) {
		SYSLOG(LOG_ERR, "%s:%d - %s\n", config_error_file(svcKernelGetCfg()), config_error_line(svcKernelGetCfg()), config_error_text(svcKernelGetCfg()));
		goto __exit;
	}
	if(!config_lookup_string(svcKernelGetCfg(), "NIUCHACZ.deviceName", &g_Main._p_deviceName)) {
		SYSLOG(LOG_ERR, "%s:%d - %s\n", config_error_file(svcKernelGetCfg()), config_error_line(svcKernelGetCfg()), config_error_text(svcKernelGetCfg()));
		goto __exit;
	}
	_status = dbStart(dbFileName, "DB_LIVE0", &g_Main._db);
	if(!KSUCCESS(_status))
		goto __exit;
	_status = schema_sync();//svcUpdateSync(getNiuchaczPcapDB());
	if(!KSUCCESS(_status))
		goto __database_stop_andexit;
	svcKernelStatus(SVC_KERNEL_STATUS_RUNNING);
	_status = cmd_sync();
	if(!KSUCCESS(_status))
		goto __database_stop_andexit;
	if(config_lookup(svcKernelGetCfg(), "NIUCHACZ.testMode") == NULL) {
		SYSLOG(LOG_INFO, "Prepare listen on device=%s", g_Main._p_deviceName);
		_status = psmgrCreateThread("niuch_pcaplsnr", "Pcap Listener", PSMGR_THREAD_USER, pcap_thread_routine, pcap_thread_cancelRoutine, NULL);
		if(!KSUCCESS(_status))
			goto __database_stop_andexit;
	} else {
		if(strcmp(argv[2], "EXPORT_FILE") == 0) {
			testExportFile(argv[3]);
		} else if(strcmp(argv[2], "IMPORT_PCAP") == 0) {
			testImportPcap(argv[3]);
                }
		svcKernelStatus(SVC_KERNEL_STATUS_STOP_PENDING);
	}
	svcKernelMainLoop();
__database_stop_andexit:
	dbStop(getNiuchaczPcapDB());
__exit:
	svcKernelExit(0);
}
