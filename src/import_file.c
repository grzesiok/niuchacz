#include "import_file.h"
#include "packet_analyze.h"
#include "svc_kernel/execute_unit/cmd_manager.h"
#include <pcap.h>
#include <sys/time.h>

int i_importFile(const char* pfile_name, PJOB_EXEC pcallback) {
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_t *pcap;
	struct pcap_pkthdr header;
	const unsigned char *packet;

	pcap = pcap_open_offline(pfile_name, errbuf);
	if (pcap == NULL) {
		SYSLOG(LOG_ERR, "error reading pcap file: %s", errbuf);
		return -1;
	}
	while ((packet = pcap_next(pcap, &header)) != NULL) {
		pcallback(header.ts, (unsigned char*)packet, header.caplen);
	}
	SYSLOG(LOG_INFO, "pcap_close");
	pcap_close(pcap);
	return 0;
}

int cmdImportFileExec(struct timeval ts, void* pdata, size_t dataSize) {
	SYSLOG(LOG_INFO, "[CMDMGR][IMPORT_FILE]Exec");
	if(dataSize != sizeof(cmd_import_cfg_t))
		return -1;
	cmd_import_cfg_t* pcfg = (cmd_import_cfg_t*)pdata;
	SYSLOG(LOG_INFO, "[CMDMGR][IMPORT_FILE](%s)", pcfg->_file_name);
	return i_importFile(pcfg->_file_name, cmdPacketAnalyzeExec);
}

int cmdImportFileCreate(void) {
	SYSLOG(LOG_INFO, "[CMDMGR][IMPORT_FILE]Create");
	return 0;
}

int cmdImportFileDestroy(void) {
	SYSLOG(LOG_INFO, "[CMDMGR][IMPORT_FILE]Destroy");
	return 0;
}
