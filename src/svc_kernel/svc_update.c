#include "svc_update.h"
#include "svc_lock.h"

KSTATUS svcUpdateStart(void) {
    return KSTATUS_SUCCESS;
}

void svcUpdateStop(void) {
}

KSTATUS svcUpdateRegisterChange(int versionMajor, int versionMinor, PSVCUPDATE_EXEC callback) {
    return KSTATUS_SUCCESS;
}

KSTATUS svcUpdateSync(sqlite3* db) {
    return KSTATUS_SUCCESS;
}

