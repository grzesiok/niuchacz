#ifndef _PACKET_ANALYZE_H
#define _PACKET_ANALYZE_H

#include <sys/time.h>
#include <stddef.h>

/* Public API Management Lifecycles */
int cmdPacketAnalyzeCreate(void);
int cmdPacketAnalyzeDestroy(void);

/* 
 * Core Packet Analyzer Execution Routing.
 * Gwarantuje 4 argumenty: db_handle, ts, packet_data, data_size.
 */
int cmdPacketAnalyzeExec(void *db_handle, struct timeval ts, const void *packet_data, size_t data_size);

#endif /* _PACKET_ANALYZE_H */