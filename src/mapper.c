#include "mapper.h"
#include "kernel.h"
#include <string.h>
#include <stdio.h>
#include "math.h"

const char gc_hexChars[] = "0123456789ABCDEF";

bool mapFrame(unsigned char *frame, size_t framelen, mapper_t* presults) {
    int size_ip, payload_i = 0, i;
    char *payload;

    memset(presults, 0, sizeof(mapper_t));
    /* map header to ethernet structure */
    memcpy(&presults->_ethernet, frame, sizeof(mapper_ethernet_t)+sizeof(mapper_ip_t));
    /* Ethernet frames are sent through network in Big Endian format */
    presults->_ethernet.eth_type = be16toh(presults->_ethernet.eth_type);
    presults->_ip.ip_len = be16toh(presults->_ip.ip_len);
    presults->_ip.ip_id = be16toh(presults->_ip.ip_id);
    presults->_ip.ip_off = be16toh(presults->_ip.ip_off);
    presults->_ip.ip_sum = be16toh(presults->_ip.ip_sum);
    /* map header (after applying offset) to ip structure */
    size_ip = presults->_ip.ip_ihl*4;
    if(size_ip < 20) {
        SYSLOG(LOG_ERR, "   * Invalid IP header length: %u bytes", size_ip);
        return false;
    }
    payload = (char*)(frame + sizeof(mapper_ethernet_t) + size_ip);
    for(i = 0;i < MIN(16, framelen - sizeof(mapper_ethernet_t) - size_ip);i++) {
        presults->_payload[payload_i++] = gc_hexChars[((payload[i] >> 4) & 0xf)];
        presults->_payload[payload_i++] = gc_hexChars[(payload[i] & 0xf)];
    }
    presults->_payload[payload_i] = '\0';
    return true;
}
