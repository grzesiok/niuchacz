#include "mapper.h"
#include "kernel.h"
#include <string.h>
#include <stdio.h>

bool mapFrame(unsigned char *frame, size_t framelen, PMAPPER_RESULTS presults) {
	int size_ip;
	const struct mapper_ip *ip = &presults->_ip;

	/* map header to ethernet structure */
	memcpy(&presults->_ethernet, frame, sizeof(struct mapper_ethernet));
	/* map header (after applying offset) to ip structure */
	memcpy(&presults->_ip, frame + SIZE_ETHERNET, sizeof(struct mapper_ip));
	size_ip = IP_HL(ip)*4;
	if(size_ip < 20) {
		SYSLOG(LOG_ERR, "   * Invalid IP header length: %u bytes", size_ip);
		return false;
	}
	return true;
}
