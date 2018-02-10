#ifndef _CMD_MANAGER_H
#define _CMD_MANAGER_H
#include "../svc_kernel.h"

typedef int (*PCMD_ROUTINE)(const char* argv[], int argc);

KSTATUS cmdmgrStart(void);
void cmdmgrStop(void);
KSTATUS cmdmgrAddCommand(const char* command, const char* description, PCMD_ROUTINE proutine, int version);
KSTATUS cmdmgrExec(const char* command, const char* argv[], int argc);
#endif /* _CMD_MANAGER_H */
