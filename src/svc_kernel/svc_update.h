#ifndef _SVC_UPDATE_H
#define _SVC_UPDATE_H
#include "svc_kernel.h"

typedef KSTATUS (*PSVCUPDATE_EXEC)(void);

KSTATUS svcUpdateStart(void);
void svcUpdateStop(void);
KSTATUS svcUpdateRegisterChange(int versionMajor, int versionMinor, PSVCUPDATE_EXEC callback);
KSTATUS svcUpdateSync(sqlite3* db);
#endif
