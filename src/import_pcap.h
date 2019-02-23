#ifndef _CMD_IMPORT_PCAP_H
#define _CMD_IMPORT_PCAP_H
#include "kernel.h"

typedef struct {
	char _file_name[255];
} cmd_import_cfg_t;

int cmdImportPcapExec(struct timeval ts, void* pdata, size_t dataSize);
int cmdImportPcapCreate(void);
int cmdImportPcapDestroy(void);
#endif
