#include "packet_analyze.h"
#include <netinet/ether.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "mapper.h"
#include "svc_kernel/database/database.h"
#include "algorithms.h"
#include "math.h"

static const char * cgStmtPackets =
		"insert into packets(ts_sec, ts_usec,eth_src_id,eth_dst_id,eth_type,"
		"ip_vhl,ip_tos,ip_len,ip_id,ip_off,ip_ttl,ip_p,ip_sum,ip_src_id,ip_dst_id)"
		"values (?,?,?,?,?,"
		"?,?,?,?,?,?,?,?,?,?);";
static const char * cgStmtEthPopulate =
                "select eth_id, eth_addr from eth where activeflag = 1;";
static const char * cgStmtEthUpdateActiveFlag =
                "update eth set activeflag = 0 where eth_addr = ? and activeflag = 1;";
static const char * cgStmtEthCreate =
		"insert into eth(eth_id, ts_sec, ts_usec, eth_addr, activeflag)"
		"values (?, ?, ?, ?, ?);";
static const char * cgStmtEthFetchPK =
		"select eth_id from eth where eth_addr = ? and activeflag = 1;";
static const char * cgStmtIPPopulate =
                "select ip_id, ip_addr, hostname from ip where activeflag = 1;";
static const char * cgStmtIPUpdateActiveFlag =
                "update ip set activeflag = 0 where ip_addr = ? and activeflag = 1;";
static const char * cgStmtIPCreate =
                "insert into ip(ip_id, ts_sec, ts_usec, ip_addr, hostname, activeflag)"
                "values (?, ?, ?, ?, ?, ?);";
static const char * cgStmtIPFetchPK =
		"select ip_id from ip where ip_addr = ? and activeflag = 1;";
bst_t* g_IPCache;
bst_t* g_EthCache;

// internal API

int i_getPK(void* param, sqlite3_stmt* stmt) {
    int *pPK = (int*)param;
    *pPK = sqlite3_column_int(stmt, 0);
    return 0;
}

uint64_t i_cmdPacketAnalyzeCacheEthStrToKey(struct ether_addr *ea) {
    return (uint64_t)(ea->ether_addr_octet[0] & 0xff) << 40
         | (uint64_t)(ea->ether_addr_octet[1] & 0xff) << 32
         | (uint64_t)(ea->ether_addr_octet[2] & 0xff) << 24
         | (uint64_t)(ea->ether_addr_octet[3] & 0xff) << 16
         | (uint64_t)(ea->ether_addr_octet[4] & 0xff) << 8
         | (uint64_t)(ea->ether_addr_octet[5] & 0xff);
}

int i_cmdPacketAnalyzeCacheEthPopulate(void* param, sqlite3_stmt* stmt) {
    struct ether_addr *ea;
    int ethID, ret;
    struct timespec ts;

    ea = ether_aton((const char*)sqlite3_column_text(stmt, 1));
    if(ea == NULL) {
        SYSLOG(LOG_INFO, "[CACHE_ETH]: %s not loaded - error during parsing!", sqlite3_column_text(stmt, 1));
        return 0;
    }
    ethID = sqlite3_column_int(stmt, 0);
    timerGetRealCurrentTimestamp(&ts);
    ts.tv_sec += 60;
    ret = bst_insert(g_EthCache, i_cmdPacketAnalyzeCacheEthStrToKey(ea), &ethID, sizeof(ethID), &ts);
    if(ret != sizeof(ethID))
        return 0;
    SYSLOG(LOG_INFO, "[CACHE_ETH]: %s(%d) loaded", sqlite3_column_text(stmt, 1), sqlite3_column_int(stmt, 0));
    return 0;
}

int i_cmdPacketAnalyzeCacheEthExpireCheck(uint64_t key, void* ptr, size_t nBytes) {
    return 60;
}

int i_cmdPacketAnalyzeCacheEthGet(struct ether_addr* ea) {
    KSTATUS _status;
    int ret, ethID;
    struct timespec ts;
    char buffEthStr[18];

    ret = bst_search(g_EthCache, i_cmdPacketAnalyzeCacheEthStrToKey(ea), &ethID, sizeof(ethID));
    if(ret == sizeof(ethID))
        return ethID;
    timerGetRealCurrentTimestamp(&ts);
    _status = dbTxnBegin(getNiuchaczPcapDB());
    _status = dbExec(getNiuchaczPcapDB(), cgStmtEthUpdateActiveFlag, 1,
                     DB_BIND_TEXT, ether_ntoa_r(ea, buffEthStr)); 
    _status = dbExec(getNiuchaczPcapDB(), cgStmtEthCreate, 5,
                     DB_BIND_NULL,
                     DB_BIND_INT64, ts.tv_sec,
                     DB_BIND_INT64, ts.tv_nsec,
                     DB_BIND_TEXT, ether_ntoa_r(ea, buffEthStr),
                     DB_BIND_INT, 1);
    _status = dbExecQuery(getNiuchaczPcapDB(), cgStmtEthFetchPK, 1, i_getPK, &ethID,
                          DB_BIND_TEXT, ether_ntoa_r(ea, buffEthStr));
    _status = dbTxnCommit(getNiuchaczPcapDB());
    ts.tv_sec += 60;
    bst_insert(g_EthCache, i_cmdPacketAnalyzeCacheEthStrToKey(ea), &ethID, sizeof(ethID), &ts);
    SYSLOG(LOG_INFO, "[CACHE_ETH]: %s(%d) loaded", ether_ntoa_r(ea, buffEthStr), ethID);
    return ethID;
}

int i_cmdPacketAnalyzeCacheIPPopulate(void* param, sqlite3_stmt* stmt) {
    uint64_t key;
    struct timespec ts;
    struct hostent *hp;
    struct in_addr ip;
    int ret, ipID;

    if(inet_aton((const char*)sqlite3_column_text(stmt, 1), &ip) == 0) {
        SYSLOG(LOG_INFO, "[CACHE_IP]: %s not loaded - error during parsing", sqlite3_column_text(stmt, 1));
        return 0;
    }
    hp = gethostbyaddr((const void *)&ip, sizeof(ip), AF_INET);
    if(hp == NULL || strncmp(hp->h_name, (const char*)sqlite3_column_text(stmt, 2), MAX(strlen((const char*)sqlite3_column_text(stmt, 2)), hp->h_length)) != 0) {
        SYSLOG(LOG_INFO, "[CACHE_IP]: %s not loaded - hostname was changed", sqlite3_column_text(stmt, 1));
        return 0;
    }
    ipID = sqlite3_column_int(stmt, 0);
    key = ip.s_addr;
    timerGetRealCurrentTimestamp(&ts);
    ts.tv_sec += 60;
    ret = bst_insert(g_IPCache, key, &ipID, sizeof(ipID), &ts);
    if(ret != sizeof(ipID))
        return 0;
    SYSLOG(LOG_INFO, "[CACHE_IP]: %s(%d) loaded", sqlite3_column_text(stmt, 1), sqlite3_column_int(stmt, 0));
    return 0;
}

int i_cmdPacketAnalyzeCacheIPExpireCheck(uint64_t key, void* ptr, size_t nBytes) {
    struct in_addr ip;
    struct hostent *hp;

    ip.s_addr = key;
    hp = gethostbyaddr((const void *)&ip, sizeof(ip), AF_INET);
    if(hp && strncmp(hp->h_name, ptr, MIN(nBytes, hp->h_length)) == 0) {
        return 60;
    }
    return 0;
}

int i_cmdPacketAnalyzeCacheIPGet(struct in_addr* ip) {
    KSTATUS _status;
    int ret, ipID;
    uint64_t key;
    struct timespec ts;
    struct hostent *hp;

    key = ip->s_addr;
    ret = bst_search(g_IPCache, key, &ipID, sizeof(ipID));
    if(ret == sizeof(ipID))
        return ipID;
    timerGetRealCurrentTimestamp(&ts);
    hp = gethostbyaddr((const void *)ip, sizeof(*ip), AF_INET);
    _status = dbTxnBegin(getNiuchaczPcapDB());
    _status = dbExec(getNiuchaczPcapDB(), cgStmtIPUpdateActiveFlag, 1,
                     DB_BIND_TEXT, inet_ntoa(*ip)); 
    _status = dbExec(getNiuchaczPcapDB(), cgStmtIPCreate, 6,
                     DB_BIND_NULL,
                     DB_BIND_INT64, ts.tv_sec,
                     DB_BIND_INT64, ts.tv_nsec,
                     DB_BIND_TEXT, inet_ntoa(*ip),
                     DB_BIND_TEXT, (hp) ? hp->h_name : "Host Not Found",
                     DB_BIND_INT, 1);
    _status = dbExecQuery(getNiuchaczPcapDB(), cgStmtIPFetchPK, 1, i_getPK, &ipID,
                          DB_BIND_TEXT, inet_ntoa(*ip));
    _status = dbTxnCommit(getNiuchaczPcapDB());
    ts.tv_sec += 60;
    bst_insert(g_IPCache, key, &ipID, sizeof(ipID), &ts);
    SYSLOG(LOG_INFO, "[CACHE_IP]: %s(%d) loaded", inet_ntoa(*ip), ipID);
    return ipID;
}

// external API

int cmdPacketAnalyzeExec(struct timeval ts, void* pdata, size_t dataSize) {
    MAPPER_RESULTS results;
    KSTATUS _status;
    int ethSrcID, ethDstID, ipSrcID, ipDstID;

    if(!mapFrame((unsigned char *)pdata, dataSize, &results)) {
    	SYSLOG(LOG_ERR, "Error in parsing message!");
        return -1;
    }
    ethSrcID = i_cmdPacketAnalyzeCacheEthGet((struct ether_addr*)&results._ethernet.eth_shost);
    ethDstID = i_cmdPacketAnalyzeCacheEthGet((struct ether_addr*)&results._ethernet.eth_dhost);
    ipSrcID = i_cmdPacketAnalyzeCacheIPGet(&results._ip.ip_src);
    ipDstID = i_cmdPacketAnalyzeCacheIPGet(&results._ip.ip_dst);
    if(ethSrcID == 0 || ethDstID == 0 || ipSrcID == 0 || ipDstID == 0) {
    	SYSLOG(LOG_ERR, "Error in translating message!");
        return -2;
    }
    _status = dbExec(getNiuchaczPcapDB(), cgStmtPackets, 15, 
                     //ts_sec since Epoch (1970) timestamp						  /* Timestamp of packet */
                     DB_BIND_INT64, ts.tv_sec,
                     //ts_usec /* Microseconds */
                     DB_BIND_INT64, ts.tv_usec,
                     //u_char  eth_shost[ETHER_ADDR_LEN];      /* source host address */
                     DB_BIND_INT, ethSrcID,
                     //u_char  eth_dhost[ETHER_ADDR_LEN];      /* destination host address */
                     DB_BIND_INT, ethDstID,
                     //u_short eth_type;                       /* IP? ARP? RARP? etc */
                     DB_BIND_INT, results._ethernet.eth_type,
                     //u_char  ip_vhl;                 		  /* version << 4 | header length >> 2 */
                     DB_BIND_INT, results._ip.ip_vhl,
                     //u_char  ip_tos;                 		  /* type of service */
                     DB_BIND_INT, results._ip.ip_tos,
                     //u_short ip_len;                 		  /* total length */
                     DB_BIND_INT, results._ip.ip_len,
                     //u_short ip_id;                  		  /* identification */
                     DB_BIND_INT, results._ip.ip_id,
                     //u_short ip_off;                 		  /* fragment offset field */
                     DB_BIND_INT, results._ip.ip_off,
                     //u_char  ip_ttl;                 		  /* time to live */
                     DB_BIND_INT, results._ip.ip_ttl,
                     //u_char  ip_p;                   		  /* protocol */
                     DB_BIND_INT, results._ip.ip_p,
                     //u_short ip_sum;                 		  /* checksum */
                     DB_BIND_INT, results._ip.ip_sum,
                     //struct  in_addr ip_src;  		  /* source address */
                     DB_BIND_INT, ipSrcID,
                     //struct  in_addr ip_dst;  		  /* dest address */
                     DB_BIND_INT, ipDstID);
    if(!KSUCCESS(_status))
        return -1;
    return 0;
}

int cmdPacketAnalyzeCreate(void) {
    KSTATUS _status;
    g_IPCache = bst_create(i_cmdPacketAnalyzeCacheIPExpireCheck);
    if(g_IPCache == NULL)
        return -1;
    g_EthCache = bst_create(i_cmdPacketAnalyzeCacheEthExpireCheck);
    if(g_EthCache == NULL)
        return -1;
    _status = dbExecQuery(getNiuchaczPcapDB(), cgStmtIPPopulate, 0, i_cmdPacketAnalyzeCacheIPPopulate, NULL);
    if(!KSUCCESS(_status))
        return -1;
    _status = dbExecQuery(getNiuchaczPcapDB(), cgStmtEthPopulate, 0, i_cmdPacketAnalyzeCacheEthPopulate, NULL);
    if(!KSUCCESS(_status))
        return -1;
    return 0;
}

int cmdPacketAnalyzeDestroy(void) {
    if(g_IPCache) {
        bst_destroy(g_IPCache);
        g_IPCache = NULL;
    }
    if(g_EthCache) {
        bst_destroy(g_EthCache);
        g_EthCache = NULL;
    }
    return 0;
}
