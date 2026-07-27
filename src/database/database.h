#ifndef _DATABASE_H
#define _DATABASE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct database_t database_t;

typedef enum {
    DB_STATE_MOUNTING = 0,
    DB_STATE_MOUNTED  = 1,
    DB_STATE_OPEN     = 2
} db_state_t;

#define DB_BIND_NULL  0
#define DB_BIND_INT64 1
#define DB_BIND_INT   2
#define DB_BIND_TEXT  3
#define DB_BIND_STEXT 4

/* Public API Management Lifecycles (Returns 0 on success, negative on failure) */
int dbmgr_start(void *shared_telemetry_list);
void dbmgr_stop(void);
int db_open(const char *p_shortname_8b, const char *filename, database_t **p_db);
void db_close(database_t *p_db);

/* Public API Statement Execution Routines */
int db_exec(database_t *p_db, const char *stmt, int bind_cnt, ...);
int db_exec_query(database_t *p_db, const char *stmt, int bind_cnt, int (*callback)(void*, void*), void *param, ...);

int db_txn_begin(database_t *p_db);
int db_txn_commit(database_t *p_db);
int db_txn_rollback(database_t *p_db);

const char* db_get_errmsg(const database_t *p_db);

#endif /* _DATABASE_H */