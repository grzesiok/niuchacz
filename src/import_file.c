#include "import.h"
#include "packet_analyze.h"
#include <pcap.h>
#include <sys/time.h>

int i_importFile(const char* pfile_name, import_callback_t pcallback) {
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_t *pcap;
	struct pcap_pkthdr header;
	const unsigned char *packet;
	int i;

	SYSLOG(LOG_INFO, "pcap_open_offline(%s)", pfile_name);
	pcap = pcap_open_offline(pfile_name, errbuf);
	if (pcap == NULL) {
		SYSLOG(LOG_ERR, "error reading pcap file: %s", errbuf);
		return -1;
	}
	while ((packet = pcap_next(pcap, &header)) != NULL) {
		pcallback[i](packet, header.ts, header.caplen);
	}
	SYSLOG(LOG_INFO, "pcap_close");
	pcap_close(pcap);
	return 0;
}

int cmdImportFileExec(struct timeval ts, void* pdata, size_t dataSize) {
	if(dataSize != sizeof(cmd_import_cfg_t))
		return -1;
	cmd_import_cfg_t* pcfg = (cmd_import_cfg_t*)pdata;
	return i_importFile(pcfg->_file_name, cmdPacketAnalyzeExec);
}

int cmdImportFileCreate(void) {
	return 0;
}

int cmdImportFileDestroy(void) {
	return 0;
}
