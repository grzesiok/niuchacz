#include "import.h"
#include <pcap.h>
#include "kernel.h"

int import_file(const char* pfile_name, import_callback_t *pcallback_list, int list_size) {
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_t *pcap;
	struct pcap_pkthdr header;
	const unsigned char *packet;
	int i;

	SYSLOG(LOG_INFO, "pcap_open_offline");
	pcap = pcap_open_offline(pfile_name, errbuf);
	if (pcap == NULL) {
		SYSLOG(LOG_ERR, "error reading pcap file: %s", errbuf);
		return -1;
	}
	while ((packet = pcap_next(pcap, &header)) != NULL) {
		for(i = 0;i < list_size;i++) {
			pcallback_list[i](packet, header.ts, header.caplen);
		}
	}
	SYSLOG(LOG_INFO, "pcap_close");
	pcap_close(pcap);
	return 0;
}
