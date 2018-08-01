#ifndef _CMD_IMPORT_FILE_H
#define _CMD_IMPORT_FILE_H
#include "kernel.h"

typedef struct {
	char _file_name[255];
} cmd_import_cfg_t;

int cmdImportFileExec(struct timeval ts, void* pdata, size_t dataSize);
int cmdImportFileCreate(void);
int cmdImportFileDestroy(void);
#endif
