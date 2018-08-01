#include "packet_analyze.h"
#include "mapper.h"
#include "svc_kernel/database/database.h"

struct {
	sqlite3_stmt *_stmt;
} cmd_packet_analyze_t;

static const char * cgStmt =
		"insert into packets(ts_sec, ts_usec,eth_shost,eth_dhost,eth_type,"
		"ip_vhl,ip_tos,ip_len,ip_id,ip_off,ip_ttl,ip_p,ip_sum,ip_src,ip_dst)"
		"values (?,?,?,?,?,"
		"?,?,?,?,?,?,?,?,?,?);";
static cmd_packet_analyze_t g_cmd_packetAnalyze;

int cmdPacketAnalyzeExec(struct timeval ts, void* pdata, size_t dataSize) {
    MAPPER_RESULTS results;
    struct timespec startTime;

    if(!mapFrame((unsigned char *)packet, packet_len, &results)) {
    	SYSLOG(LOG_ERR, "Error in parsing message!");
        return -1;
    }
    //ts_sec since Epoch (1970) timestamp						  /* Timestamp of packet */
    if(!dbBind_int64(true, g_cmd_packetAnalyze._stmt, 1, ts.tv_sec)) {
        return -1;
    }//ts_usec /* Microseconds */
    if(!dbBind_int64(true, g_cmd_packetAnalyze._stmt, 2, ts.tv_usec)) {
        return -1;
    }
    //u_char  eth_shost[ETHER_ADDR_LEN];      /* source host address */
    if(!dbBind_text(true, g_cmd_packetAnalyze._stmt, 3, ether_ntoa((struct ether_addr*)&results._ethernet.eth_shost)) != SQLITE_OK) {
        return -1;
    }
    //u_char  eth_dhost[ETHER_ADDR_LEN];      /* destination host address */
    if(!dbBind_text(true, g_cmd_packetAnalyze._stmt, 4, ether_ntoa((struct ether_addr*)&results._ethernet.eth_dhost)) != SQLITE_OK) {
        return -1;
    }
    //u_short eth_type;                       /* IP? ARP? RARP? etc */
    if(!dbBind_int(true, g_cmd_packetAnalyze._stmt, 5, results._ethernet.eth_type) != SQLITE_OK) {
        return -1;
    }
    //u_char  ip_vhl;                 		  /* version << 4 | header length >> 2 */
    if(!dbBind_int(true, g_cmd_packetAnalyze._stmt, 6, results._ip.ip_vhl) != SQLITE_OK) {
        return -1;
    }
    //u_char  ip_tos;                 		  /* type of service */
    if(!dbBind_int(true, g_cmd_packetAnalyze._stmt, 7, results._ip.ip_tos) != SQLITE_OK) {
        return -1;
    }
    //u_short ip_len;                 		  /* total length */
    if(!dbBind_int(true, g_cmd_packetAnalyze._stmt, 8, results._ip.ip_len) != SQLITE_OK) {
        return -1;
    }
    //u_short ip_id;                  		  /* identification */
    if(!dbBind_int(true, g_cmd_packetAnalyze._stmt, 9, results._ip.ip_id) != SQLITE_OK) {
        return -1;
    }
    //u_short ip_off;                 		  /* fragment offset field */
    if(!dbBind_int(true, g_cmd_packetAnalyze._stmt, 10, results._ip.ip_off) != SQLITE_OK) {
        return -1;
    }
    //u_char  ip_ttl;                 		  /* time to live */
    if(!dbBind_int(true, g_cmd_packetAnalyze._stmt, 11, results._ip.ip_ttl) != SQLITE_OK) {
        return -1;
    }
    //u_char  ip_p;                   		  /* protocol */
    if(!dbBind_int(true, g_cmd_packetAnalyze._stmt, 12, results._ip.ip_p) != SQLITE_OK) {
        return -1;
    }
    //u_short ip_sum;                 		  /* checksum */
    if(!dbBind_int(true, g_cmd_packetAnalyze._stmt, 13, results._ip.ip_sum) != SQLITE_OK) {
        return -1;
    }
    //struct  in_addr ip_src;  		  /* source address */
    if(!dbBind_text(true, g_cmd_packetAnalyze._stmt, 14, inet_ntoa(results._ip.ip_src)) != SQLITE_OK) {
        return -1;
    }
    //struct  in_addr ip_dst;  		  /* dest address */
    if(!dbBind_text(true, g_cmd_packetAnalyze._stmt, 15, inet_ntoa(results._ip.ip_dst)) != SQLITE_OK) {
        return -1;
    }
	startTime = timerStart();
    if(sqlite3_step(g_cmd_packetAnalyze._stmt) != SQLITE_DONE) {
    	SYSLOG(LOG_ERR, "%s", dbGetErrmsg(g_cmd_packetAnalyze._db));
    }
    sqlite3_reset(g_cmd_packetAnalyze._stmt);
    statsUpdate(g_statsKey_DbExecTime, timerStop(startTime));
    return 0;
}

int cmdPacketAnalyzeCreate(void) {
    if(sqlite3_prepare(g_Main._db, cgStmt, -1, &g_cmd_packetAnalyze._stmt, 0) != SQLITE_OK) {
        SYSLOG(LOG_ERR, "Could not prepare statement.");
        return -1;
    }
    return 0;
}

int cmdaPacketAnalyzeDestroy(void) {
    sqlite3_finalize(g_cmd_packetAnalyze._stmt);
    return 0;
}
