#ifndef _CMD_EXPORT_FILE_H
#define _CMD_EXPORT_FILE_H
#include "kernel.h"

typedef struct {
	char _file_name[255];
} cmd_export_cfg_t;

int cmdExportFileExec(struct timeval ts, void* pdata, size_t dataSize);
int cmdExportFileCreate(void);
int cmdExportFileDestroy(void);
#endif
