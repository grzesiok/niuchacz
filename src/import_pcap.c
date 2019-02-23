#include "import_pcap.h"
#include "packet_analyze.h"
#include "svc_kernel/execute_unit/cmd_manager.h"
#include <pcap.h>
#include <sys/time.h>

int i_importPcap(const char* pfile_name, PJOB_EXEC pcallback) {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *pcap;
    struct pcap_pkthdr header;
    const unsigned char *packet;

    pcap = pcap_open_offline(pfile_name, errbuf);
    if (pcap == NULL) {
        SYSLOG(LOG_ERR, "[CMDMGR][IMPORT_PCAP] error reading pcap file: %s", errbuf);
        return -1;
    }
    while ((packet = pcap_next(pcap, &header)) != NULL) {
        pcallback(header.ts, (unsigned char*)packet, header.caplen);
    }
    SYSLOG(LOG_INFO, "[CMDMGR][IMPORT_PCAP] pcap_close");
    pcap_close(pcap);
    return 0;
}

int cmdImportPcapExec(struct timeval ts, void* pdata, size_t dataSize) {
    if(dataSize != sizeof(cmd_import_cfg_t))
        return -1;
    cmd_import_cfg_t* pcfg = (cmd_import_cfg_t*)pdata;
    SYSLOG(LOG_INFO, "[CMDMGR][IMPORT_PCAP](%s)", pcfg->_file_name);
    return i_importPcap(pcfg->_file_name, cmdPacketAnalyzeExec);
}

int cmdImportPcapCreate(void) {
    return 0;
}

int cmdImportPcapDestroy(void) {
    return 0;
}
