#include "mapper.h"
#include "kernel.h"
#include <string.h>
#include <stdio.h>
#include "math.h"

const char gc_hexChars[] = "0123456789ABCDEF";

bool mapFrame(unsigned char *frame, size_t framelen, PMAPPER_RESULTS presults) {
    int size_ip, payload_i = 0, i;
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
    for(i = 0;i < MIN(16, framelen - SIZE_ETHERNET - size_ip);i++) {
        presults->_payload[payload_i++] = gc_hexChars[((payload[i] >> 4) & 0xf)];
        presults->_payload[payload_i++] = gc_hexChars[(payload[i] & 0xf)];
    }
    presults->_payload[payload_i] = '\0';
    return true;
}
