#ifndef _CMD_PACKET_ANALYZE_H
#define _CMD_PACKET_ANALYZE_H
#include "kernel.h"

int cmdPacketAnalyzeExec(struct timeval ts, void* pdata, size_t dataSize);
int cmdPacketAnalyzeCreate(void);
int cmdPacketAnalyzeDestroy(void);
#endif
