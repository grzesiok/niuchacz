#ifndef _CMD_MANAGER_H
#define _CMD_MANAGER_H
#include "../svc_kernel.h"

typedef int (*PCMD_ROUTINE)(const char* argv[], int argc);

KSTATUS cmdmgrStart(void);
void cmdmgrStop(void);
PCMD_ROUTINE cmdmgrFindRoutine(const char* pcmdText);
#endif /* _CMD_MANAGER_H */
