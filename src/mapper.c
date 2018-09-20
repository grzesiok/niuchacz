#include "mapper.h"
#include "kernel.h"
#include <string.h>
#include <stdio.h>

bool mapFrame(unsigned char *frame, size_t framelen, PMAPPER_RESULTS presults) {
	int size_ip;
	const struct mapper_ip *ip = &presults->_ip;
        char *payload;

	/* map header to ethernet structure */
	memcpy(&presults->_ethernet, frame, sizeof(struct mapper_ethernet));
	/* map header (after applying offset) to ip structure */
	memcpy(&presults->_ip, frame + SIZE_ETHERNET, sizeof(struct mapper_ip));
	size_ip = IP_HL(ip)*4;
	if(size_ip < 20) {
		SYSLOG(LOG_ERR, "   * Invalid IP header length: %u bytes", size_ip);
		return false;
	}
        payload = (char*)(frame + SIZE_ETHERNET + size_ip);
        sprintf(presults->_payload_16b, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                payload[0], payload[1], payload[2], payload[3], payload[4], payload[5], payload[6], payload[7],
                payload[8], payload[9], payload[10], payload[11], payload[12], payload[13], payload[14], payload[15]);
	return true;
}
