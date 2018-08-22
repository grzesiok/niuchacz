#include "svc_kernel/svc_update.h"
#include "svc_kernel/svc_lock.h"

//update_change - list of changes
//update_status - current version
static const char * cgCreateSchema =
		"create table if not exists update_changes ("
		"ts_sec unsigned big int, ts_usec unsigned big int, eth_shost text, eth_dhost text, eth_type int,"
		"ip_vhl int,ip_tos int,ip_len int,ip_id int,ip_off int,ip_ttl int,ip_p int,ip_sum int,ip_src text,ip_dst text"
		");";

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

