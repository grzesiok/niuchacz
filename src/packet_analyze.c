#include "packet_analyze.h"
#include <netinet/ether.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "mapper.h"
#include "svc_kernel/database/database.h"

static const char * cgStmt =
		"insert into packets(ts_sec, ts_usec,eth_shost,eth_dhost,eth_type,"
		"ip_vhl,ip_tos,ip_len,ip_id,ip_off,ip_ttl,ip_p,ip_sum,ip_src,ip_dst)"
		"values (?,?,?,?,?,"
		"?,?,?,?,?,?,?,?,?,?);";

int cmdPacketAnalyzeExec(struct timeval ts, void* pdata, size_t dataSize) {
    MAPPER_RESULTS results;
    KSTATUS _status;

    if(!mapFrame((unsigned char *)pdata, dataSize, &results)) {
    	SYSLOG(LOG_ERR, "Error in parsing message!");
        return -1;
    }
    _status = dbExec(getNiuchaczPcapDB(), cgStmt, 15, 
                     //ts_sec since Epoch (1970) timestamp						  /* Timestamp of packet */
                     DB_BIND_INT64, ts.tv_sec,
                     //ts_usec /* Microseconds */
                     DB_BIND_INT64, ts.tv_usec,
                     //u_char  eth_shost[ETHER_ADDR_LEN];      /* source host address */
                     DB_BIND_TEXT, ether_ntoa((struct ether_addr*)&results._ethernet.eth_shost),
                     //u_char  eth_dhost[ETHER_ADDR_LEN];      /* destination host address */
                     DB_BIND_TEXT, ether_ntoa((struct ether_addr*)&results._ethernet.eth_dhost),
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
                     DB_BIND_TEXT, inet_ntoa(results._ip.ip_src),
                     //struct  in_addr ip_dst;  		  /* dest address */
                     DB_BIND_TEXT, inet_ntoa(results._ip.ip_dst));
    if(!KSUCCESS(_status))
        return -1;
    return 0;
}

int cmdPacketAnalyzeCreate(void) {
    return 0;
}

int cmdPacketAnalyzeDestroy(void) {
    return 0;
}
