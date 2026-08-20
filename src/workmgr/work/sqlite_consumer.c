/*
 * src/work/sqlite_consumer.c
 * SQLite Database Pipeline Consumer Work Implementation (ANSI C)
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include <sqlite3.h>
#include "workmgr/workmgr.h"
#include "algorithms/queue/queue.h"
#include <libconfig.h>
#include "database/database.h"

#define SQLITE_CONSUMER_BUFFER_SIZE 65535

static int i_sqlite_consumer_fetch_id_callback(void *param, void *stmt)
{
    int64_t *out_id = (int64_t *)param;
    *out_id = (int64_t)sqlite3_column_int64((sqlite3_stmt *)stmt, 0);
    return 0;
}

static int64_t i_sqlite_consumer_resolve_eth_id(database_t *db, struct timeval ts, const char *mac_str)
{
    int64_t eth_id = -1;

    db_exec_query(db, "SELECT eth_id FROM eth WHERE eth_addr = ?;", 1,
                  i_sqlite_consumer_fetch_id_callback, &eth_id,
                  DB_BIND_TEXT, mac_str);

    if (eth_id == -1)
    {
        db_exec(db, "INSERT INTO eth(ts_sec, ts_usec, eth_addr, activeflag) VALUES (?, ?, ?, 1);", 3,
                DB_BIND_INT64, (uint64_t)ts.tv_sec,
                DB_BIND_INT, (int)ts.tv_usec,
                DB_BIND_TEXT, mac_str);

        db_exec_query(db, "SELECT last_insert_rowid();", 0,
                      i_sqlite_consumer_fetch_id_callback, &eth_id);
    }

    return eth_id;
}

static int64_t i_sqlite_consumer_resolve_ip_id(database_t *db, struct timeval ts, const char *ip_str)
{
    int64_t ip_id = -1;

    db_exec_query(db, "SELECT ip_id FROM ip WHERE ip_addr = ?;", 1,
                  i_sqlite_consumer_fetch_id_callback, &ip_id,
                  DB_BIND_TEXT, ip_str);

    if (ip_id == -1)
    {
        db_exec(db, "INSERT INTO ip(ts_sec, ts_usec, ip_addr, hostname, activeflag) VALUES (?, ?, ?, 'unknown', 1);", 4,
                DB_BIND_INT64, (uint64_t)ts.tv_sec,
                DB_BIND_INT, (int)ts.tv_usec,
                DB_BIND_TEXT, ip_str,
                DB_BIND_TEXT, "unknown");

        db_exec_query(db, "SELECT last_insert_rowid();", 0,
                      i_sqlite_consumer_fetch_id_callback, &ip_id);
    }

    return ip_id;
}

static int i_sqlite_consumer_analyze_packet(database_t *db, struct timeval ts, const void *packet_data, size_t data_size)
{
    if (db == NULL || packet_data == NULL || data_size < sizeof(struct ether_header))
    {
        return -1;
    }

    const struct ether_header *eth = (const struct ether_header *)packet_data;

    char src_mac_str[18];
    char dst_mac_str[18];

    snprintf(src_mac_str, sizeof(src_mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
             eth->ether_shost[0], eth->ether_shost[1], eth->ether_shost[2],
             eth->ether_shost[3], eth->ether_shost[4], eth->ether_shost[5]);
    snprintf(dst_mac_str, sizeof(dst_mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
             eth->ether_dhost[0], eth->ether_dhost[1], eth->ether_dhost[2],
             eth->ether_dhost[3], eth->ether_dhost[4], eth->ether_dhost[5]);

    uint16_t ether_type = ntohs(eth->ether_type);
    if (ether_type != ETHERTYPE_IP)
    {
        return 0;
    }

    size_t ip_offset = sizeof(struct ether_header);
    if (data_size < ip_offset + sizeof(struct iphdr))
    {
        return -1;
    }

    const struct iphdr *ip = (const struct iphdr *)((uintptr_t)packet_data + ip_offset);
    struct in_addr src_addr = {.s_addr = ip->saddr};
    struct in_addr dst_addr = {.s_addr = ip->daddr};

    char src_ip_str[INET_ADDRSTRLEN];
    char dst_ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &src_addr, src_ip_str, sizeof(src_ip_str));
    inet_ntop(AF_INET, &dst_addr, dst_ip_str, sizeof(dst_ip_str));

    size_t ip_header_len = ip->ihl * 4;
    size_t payload_offset = ip_offset + ip_header_len;
    const char *payload_ptr = "NULL";
    char *allocated_payload_hex = NULL;

    if (data_size > payload_offset)
    {
        size_t payload_len = data_size - payload_offset;
        allocated_payload_hex = (char *)malloc((payload_len * 2) + 1);
        if (allocated_payload_hex != NULL)
        {
            for (size_t i = 0; i < payload_len; i++)
            {
                const uint8_t *b = (const uint8_t *)((uintptr_t)packet_data + payload_offset + i);
                snprintf(&allocated_payload_hex[i * 2], 3, "%02x", *b);
            }
            payload_ptr = allocated_payload_hex;
        }
    }

    int64_t eth_src_id = i_sqlite_consumer_resolve_eth_id(db, ts, src_mac_str);
    int64_t eth_dst_id = i_sqlite_consumer_resolve_eth_id(db, ts, dst_mac_str);
    int64_t ip_src_id = i_sqlite_consumer_resolve_ip_id(db, ts, src_ip_str);
    int64_t ip_dst_id = i_sqlite_consumer_resolve_ip_id(db, ts, dst_ip_str);

    int status = db_exec(db, "INSERT INTO packets (ts_sec, ts_usec, eth_src_id, eth_dst_id, eth_type, "
                             "ip_vhl, ip_tos, ip_len, ip_id, ip_off, ip_ttl, ip_p, ip_sum, "
                             "ip_src_id, ip_dst_id, payload) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                         16,
                         DB_BIND_INT64, (uint64_t)ts.tv_sec,
                         DB_BIND_INT, (int)ts.tv_usec,
                         DB_BIND_INT64, (uint64_t)eth_src_id,
                         DB_BIND_INT64, (uint64_t)eth_dst_id,
                         DB_BIND_INT, (int)ether_type,
                         DB_BIND_INT, (int)((ip->version << 4) | ip->ihl),
                         DB_BIND_INT, (int)ip->tos,
                         DB_BIND_INT, (int)ntohs(ip->tot_len),
                         DB_BIND_INT, (int)ntohs(ip->id),
                         DB_BIND_INT, (int)ntohs(ip->frag_off),
                         DB_BIND_INT, (int)ip->ttl,
                         DB_BIND_INT, (int)ip->protocol,
                         DB_BIND_INT, (int)ntohs(ip->check),
                         DB_BIND_INT64, (uint64_t)ip_src_id,
                         DB_BIND_INT64, (uint64_t)ip_dst_id,
                         DB_BIND_TEXT, payload_ptr);

    if (allocated_payload_hex != NULL)
    {
        free(allocated_payload_hex);
    }

    return (status == 0) ? 0 : -1;
}

static void i_sqlite_consumer_setup(void *raw_ctx)
{
    WorkContext_t *ctx = (WorkContext_t *)raw_ctx;
    if (!ctx || !ctx->custom_config)
    {
        fprintf(stderr, "[SQLITE_CONSUMER][FATAL] Work configuration section missing.\n");
        exit(EXIT_FAILURE);
    }

    /* Expect custom_config to be a config_setting_t* pointing to works.sqlite_consumer */
    config_setting_t *setting = (config_setting_t *)ctx->custom_config;
    const char *db_filename = NULL;
    if (config_setting_lookup_string(setting, "database_file", &db_filename) == CONFIG_FALSE || db_filename == NULL || db_filename[0] == '\0')
    {
        fprintf(stderr, "[SQLITE_CONSUMER][FATAL] 'database_file' not set in work config.\n");
        exit(EXIT_FAILURE);
    }

    printf("[SQLITE_CONSUMER] Initializing worker node bound to storage backend: %s\n", db_filename);

    /* Open database and store handle in local state */
    database_t *db = NULL;
    if (db_open("SQLC", db_filename, &db) != 0)
    {
        fprintf(stderr, "[SQLITE_CONSUMER][FATAL] Failed to open database '%s'.\n", db_filename);
        exit(EXIT_FAILURE);
    }

    /* Ensure required schema exists: packets, eth, ip tables. Create if missing. */
    const char *create_packets =
        "CREATE TABLE IF NOT EXISTS packets ("
        "packet_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "ts_sec INTEGER, ts_usec INTEGER,"
        "eth_src_id INTEGER, eth_dst_id INTEGER, eth_type INTEGER,"
        "ip_vhl INTEGER, ip_tos INTEGER, ip_len INTEGER, ip_id INTEGER, ip_off INTEGER,"
        "ip_ttl INTEGER, ip_p INTEGER, ip_sum INTEGER,"
        "ip_src_id INTEGER, ip_dst_id INTEGER, payload TEXT"
        ");";

    const char *create_eth =
        "CREATE TABLE IF NOT EXISTS eth ("
        "eth_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "ts_sec INTEGER, ts_usec INTEGER,"
        "eth_addr TEXT UNIQUE, activeflag INTEGER"
        ");";

    const char *create_ip =
        "CREATE TABLE IF NOT EXISTS ip ("
        "ip_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "ts_sec INTEGER, ts_usec INTEGER,"
        "ip_addr TEXT UNIQUE, hostname TEXT, activeflag INTEGER" 
        ");";

    if (db_exec(db, create_packets, 0) != 0) {
        fprintf(stderr, "[SQLITE_CONSUMER][FATAL] Failed to ensure 'packets' table: %s\n", db_get_errmsg(db));
        db_close(db);
        exit(EXIT_FAILURE);
    }

    if (db_exec(db, create_eth, 0) != 0) {
        fprintf(stderr, "[SQLITE_CONSUMER][FATAL] Failed to ensure 'eth' table: %s\n", db_get_errmsg(db));
        db_close(db);
        exit(EXIT_FAILURE);
    }

    if (db_exec(db, create_ip, 0) != 0) {
        fprintf(stderr, "[SQLITE_CONSUMER][FATAL] Failed to ensure 'ip' table: %s\n", db_get_errmsg(db));
        db_close(db);
        exit(EXIT_FAILURE);
    }

    /* Replace custom_config with our local db handle state for the child */
    ctx->custom_config = (void *)db;

    /* Increment consumer metrics registration maps across the active shared queue */
    queue_consumer_new(ctx->pipeline);
}

static void i_sqlite_consumer_run_loop(void *raw_ctx)
{
    WorkContext_t *ctx = (WorkContext_t *)raw_ctx;
    void *local_msg = NULL;

    while (1)
    {
        /* Read packets out of the process-shared queue segment (blocks while empty) */
        int read_bytes = queue_read(ctx->pipeline, &local_msg, NULL);

        if (read_bytes == QUEUE_RET_DESTROYING)
        {
            printf("[SQLITE_CONSUMER] Termination requested by parent node. Retracting worker allocation scopes.\n");
            break;
        }

        if (read_bytes > 0)
        {
            database_t *db = (database_t *)ctx->custom_config;
            if (db != NULL)
            {
                struct timeval ts;
                gettimeofday(&ts, NULL);
                int status = i_sqlite_consumer_analyze_packet(db, ts, local_msg, (size_t)read_bytes);
                if (status != 0)
                {
                    /* prepare a short hex prefix for debugging */
                    size_t prefix_len = (size_t)read_bytes;
                    if (prefix_len > 32)
                        prefix_len = 32;
                    char hex_prefix[65];
                    size_t hp = 0;
                    if (local_msg != NULL && prefix_len > 0)
                    {
                        const unsigned char *b = (const unsigned char *)local_msg;
                        for (size_t i = 0; i < prefix_len && hp + 2 < sizeof(hex_prefix); i++)
                        {
                            snprintf(&hex_prefix[hp], 3, "%02x", b[i]);
                            hp += 2;
                        }
                    }
                    hex_prefix[hp] = '\0';

                    fprintf(stderr, "[SQLITE_CONSUMER][WARN] Failed to store packet payload in SQLite (status=%d, bytes=%d, errmsg=\"%s\", prefix=%s)\n",
                            status, read_bytes, db_get_errmsg(db), (hp > 0) ? hex_prefix : "");
                }
            }
            free(local_msg);
            local_msg = NULL;
        }
    }

    /* local_msg is freed after processing each entry */
}

static void i_sqlite_consumer_teardown(void *raw_ctx)
{
    WorkContext_t *ctx = (WorkContext_t *)raw_ctx;
    if (!ctx)
        return;
    printf("[SQLITE_CONSUMER] Database pipeline workspace detached safely.\n");
    /* Close and free database handle if present */
    database_t *db = (database_t *)ctx->custom_config;
    if (db)
        db_close(db);
    queue_consumer_free(ctx->pipeline);
}

/* Public global registration descriptor reference used inside main.c */
WorkDescriptor_t sqlite_consumer_work = {
    .work_type_name = "SQLITE_CONSUMER",
    .config_name = "sqlite_consumer",
    .work_setup = i_sqlite_consumer_setup,
    .work_run_loop = i_sqlite_consumer_run_loop,
    .work_teardown = i_sqlite_consumer_teardown};