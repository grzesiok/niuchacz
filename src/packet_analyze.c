#include "packet_analyze.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/syslog.h>

#include "sqlite3.h" /* FIXED: Added missing header for SQLite internal types and macros */
#include "database/database.h"

int cmdPacketAnalyzeCreate(void) {
    return 0;
}

int cmdPacketAnalyzeDestroy(void) {
    return 0;
}

// ====================================================================
// INTERNAL API: DATABASE ID RESOLVERS
// ====================================================================

static int i_analyze_fetch_id_callback(void *param, void *stmt) {
    int64_t *out_id = (int64_t*)param;
    /* FIXED: Fully resolved type cast for the column index parameter */
    *out_id = (int64_t)sqlite3_column_int64((sqlite3_stmt*)stmt, 0);
    return 0;
}

static int64_t i_analyze_resolve_eth_id(database_t *db, struct timeval ts, const char *mac_str) {
    int64_t eth_id = -1;
    
    db_exec_query(db, "SELECT eth_id FROM eth WHERE eth_addr = ?;", 1, i_analyze_fetch_id_callback, &eth_id, DB_BIND_TEXT, mac_str);
    
    if (eth_id == -1) {
        db_exec(db, "INSERT INTO eth(ts_sec, ts_usec, eth_addr, activeflag) VALUES (?, ?, ?, 1);", 3,
                DB_BIND_INT64, (uint64_t)ts.tv_sec,
                DB_BIND_INT,   (int)ts.tv_usec,
                DB_BIND_TEXT,  mac_str);
                
        db_exec_query(db, "SELECT last_insert_rowid();", 0, i_analyze_fetch_id_callback, &eth_id);
    }
    
    return eth_id;
}

static int64_t i_analyze_resolve_ip_id(database_t *db, struct timeval ts, const char *ip_str) {
    int64_t ip_id = -1;
    
    db_exec_query(db, "SELECT ip_id FROM ip WHERE ip_addr = ?;", 1, i_analyze_fetch_id_callback, &ip_id, DB_BIND_TEXT, ip_str);
    
    if (ip_id == -1) {
        db_exec(db, "INSERT INTO ip(ts_sec, ts_usec, ip_addr, hostname, activeflag) VALUES (?, ?, ?, 'unknown', 1);", 4,
                DB_BIND_INT64, (uint64_t)ts.tv_sec,
                DB_BIND_INT,   (int)ts.tv_usec,
                DB_BIND_TEXT,  ip_str,
                DB_BIND_TEXT,  "unknown");
                
        db_exec_query(db, "SELECT last_insert_rowid();", 0, i_analyze_fetch_id_callback, &ip_id);
    }
    
    return ip_id;
}

// ====================================================================
// CORE PARSING ENGINE: NETWORK BINARY LAYERS UNMARSHALLER
// ====================================================================

/* FIXED: Reconciled structure layout parameters to fit the unified 4-argument signature */
int cmdPacketAnalyzeExec(void *db_handle, struct timeval ts, const void *packet_data, size_t data_size) {
    database_t *db = (database_t*)db_handle;
    if (db == NULL || packet_data == NULL || data_size < sizeof(struct ether_header)) {
        return -1;
    }

    const struct ether_header *eth = (const struct ether_header*)packet_data;
    
    /* FIXED: Upgraded from single char scalars to actual standard string buffers lengths */
    char src_mac_str[18];
    char dst_mac_str[18];
    
    /* FIXED: Corrected multi-index mapping properties array values */
    snprintf(src_mac_str, sizeof(src_mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
             eth->ether_shost[0], eth->ether_shost[1], eth->ether_shost[2],
             eth->ether_shost[3], eth->ether_shost[4], eth->ether_shost[5]);
             
    snprintf(dst_mac_str, sizeof(dst_mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
             eth->ether_dhost[0], eth->ether_dhost[1], eth->ether_dhost[2],
             eth->ether_dhost[3], eth->ether_dhost[4], eth->ether_dhost[5]);

    uint16_t ether_type = ntohs(eth->ether_type);
    if (ether_type != ETHERTYPE_IP) {
        return 0; /* Bypass non-IP structures cleanly */
    }

    size_t ip_offset = sizeof(struct ether_header);
    if (data_size < ip_offset + sizeof(struct iphdr)) {
        return -1;
    }

    const struct iphdr *ip = (const struct iphdr*)((uintptr_t)packet_data + ip_offset);
    
    struct in_addr src_addr = { .s_addr = ip->saddr };
    struct in_addr dst_addr = { .s_addr = ip->daddr };
    
    char src_ip_str[INET_ADDRSTRLEN];
    char dst_ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &src_addr, src_ip_str, sizeof(src_ip_str));
    inet_ntop(AF_INET, &dst_addr, dst_ip_str, sizeof(dst_ip_str));

    size_t ip_header_len = ip->ihl * 4;
    size_t payload_offset = ip_offset + ip_header_len;
    const char *payload_ptr = "NULL";
    char *allocated_payload_hex = NULL;

    if (data_size > payload_offset) {
        size_t payload_len = data_size - payload_offset;
        allocated_payload_hex = (char*)malloc((payload_len * 2) + 1);
        if (allocated_payload_hex != NULL) {
            for (size_t i = 0; i < payload_len; i++) {
                const uint8_t *b = (const uint8_t*)((uintptr_t)packet_data + payload_offset + i);
                snprintf(&allocated_payload_hex[i * 2], 3, "%02x", *b);
            }
            payload_ptr = allocated_payload_hex;
        }
    }

    /* Resolve entities indices across relative database tables */
    int64_t eth_src_id = i_analyze_resolve_eth_id(db, ts, src_mac_str);
    int64_t eth_dst_id = i_analyze_resolve_eth_id(db, ts, dst_mac_str);
    int64_t ip_src_id  = i_analyze_resolve_ip_id(db, ts, src_ip_str);
    int64_t ip_dst_id  = i_analyze_resolve_ip_id(db, ts, dst_ip_str);

    int status = db_exec(db, "INSERT INTO packets (ts_sec, ts_usec, eth_src_id, eth_dst_id, eth_type, "
                             "ip_vhl, ip_tos, ip_len, ip_id, ip_off, ip_ttl, ip_p, ip_sum, "
                             "ip_src_id, ip_dst_id, payload) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", 16,
                     DB_BIND_INT64, (uint64_t)ts.tv_sec,
                     DB_BIND_INT,   (int)ts.tv_usec,
                     DB_BIND_INT64, (uint64_t)eth_src_id,
                     DB_BIND_INT64, (uint64_t)eth_dst_id,
                     DB_BIND_INT,   (int)ether_type,
                     DB_BIND_INT,   (int)((ip->version << 4) | ip->ihl),
                     DB_BIND_INT,   (int)ip->tos,
                     DB_BIND_INT,   (int)ntohs(ip->tot_len),
                     DB_BIND_INT,   (int)ntohs(ip->id),
                     DB_BIND_INT,   (int)ntohs(ip->frag_off),
                     DB_BIND_INT,   (int)ip->ttl,
                     DB_BIND_INT,   (int)ip->protocol,
                     DB_BIND_INT,   (int)ntohs(ip->check),
                     DB_BIND_INT64, (uint64_t)ip_src_id,
                     DB_BIND_INT64, (uint64_t)ip_dst_id,
                     DB_BIND_TEXT,  payload_ptr);

    if (allocated_payload_hex != NULL) {
        free(allocated_payload_hex);
    }

    return (status == 0) ? 0 : -1;
}