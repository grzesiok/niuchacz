#include "import.h"
#include "../sqlite/sqlite3.h"
#include <pcap.h>
#include "kernel.h"
#include "ocilib.h"

int import_file(const char* pfile_name, import_callback_t *pcallback_list, int list_size) {
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_t *pcap;
	struct pcap_pkthdr header;
	const unsigned char *packet;
	int i;

	SYSLOG(LOG_INFO, "pcap_open_offline");
	pcap = pcap_open_offline(pfile_name, errbuf);
	if (pcap == NULL) {
		SYSLOG(LOG_ERR, "error reading pcap file: %s", errbuf);
		return -1;
	}
	while ((packet = pcap_next(pcap, &header)) != NULL) {
		for(i = 0;i < list_size;i++) {
			pcallback_list[i](packet, header.ts, header.caplen);
		}
	}
	SYSLOG(LOG_INFO, "pcap_close");
	pcap_close(pcap);
	return 0;
}

void err_handler(OCI_Error *err)
{
    printf("%s\n", OCI_ErrorGetString(err));
}

OCI_Statement* st;
OCI_Connection* cn;
int i_import_db_callback(void *NotUsed, int argc, char **argv, char **azColName) {
	OCI_Error *err;
	OCI_Timestamp *tm;
	int i;

    for(i = 0; i < argc; i++) {
        printf("%s = %s ", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    OCI_Prepare(st, "insert into netdumps "
    				"(ts,"
    				" eth_shost,"
					" eth_dhost,"
					" eth_type,"
					" ip_vhl,"
					" ip_tos,"
					" ip_len,"
					" ip_id,"
					" ip_off,"
					" ip_ttl,"
					" ip_p,"
					" ip_sum,"
					" ip_src,"
					" ip_dst)"
					" values(:0, :1, :2, :3, :4, :5, :6, :7, :8, :9, :10, :11, :12, :13)");
    tm = OCI_TimestampCreate(NULL, OCI_TIMESTAMP);
    OCI_TimestampSysTimeStamp(tm);
    OCI_BindTimestamp(st, ":0", tm);
    OCI_BindString(st, ":1", (otext*)argv[2], strlen(argv[2])+1);
    OCI_BindString(st, ":2", (otext*)argv[3], strlen(argv[3])+1);
    OCI_BindString(st, ":3", (otext*)argv[4], strlen(argv[4])+1);
    OCI_BindString(st, ":4", (otext*)argv[5], strlen(argv[5])+1);
    OCI_BindString(st, ":5", (otext*)argv[6], strlen(argv[6])+1);
    OCI_BindString(st, ":6", (otext*)argv[7], strlen(argv[7])+1);
    OCI_BindString(st, ":7", (otext*)argv[8], strlen(argv[8])+1);
    OCI_BindString(st, ":8", (otext*)argv[9], strlen(argv[9])+1);
    OCI_BindString(st, ":9", (otext*)argv[10], strlen(argv[10])+1);
    OCI_BindString(st, ":10", (otext*)argv[11], strlen(argv[11])+1);
    OCI_BindString(st, ":11", (otext*)argv[12], strlen(argv[12])+1);
    OCI_BindString(st, ":12", (otext*)argv[13], strlen(argv[13])+1);
    OCI_BindString(st, ":13", (otext*)argv[14], strlen(argv[14])+1);
    printf("Oracle Insert\n");
    if(!OCI_Execute(st))
    {
        printf("Number of DML array errors : %d\n", OCI_GetBatchErrorCount(st));
        err = OCI_GetBatchError(st);
        while (err)
        {
            printf("Error at row %d : %s\n", OCI_ErrorGetRow(err), OCI_ErrorGetString(err));
            err = OCI_GetBatchError(st);
        }
    } else {
    	printf("%d row inserted\n", OCI_GetAffectedRows(st));
    }
    OCI_TimestampFree(tm);
    OCI_Commit(cn);
    return 0;
}

int import_db(const char* pdbfile_name, const char* db_connection, const char* db_user, const char* db_user_password) {
	int rc;
	sqlite3* db;
	char *err_msg = 0;

	rc = sqlite3_open(pdbfile_name, &db);
	printf("Open SQLITE3\n");
	if(rc)
		return rc;
	printf("Init OCILIB\n");
    if(!OCI_Initialize(err_handler, NULL, OCI_ENV_DEFAULT))
    	return 1;

    cn = OCI_ConnectionCreate(db_connection, db_user, db_user_password, OCI_SESSION_DEFAULT);
    st = OCI_StatementCreate(cn);

	rc = sqlite3_exec(db, "select null as ts_sec,"
								" null as ts_usec,"
								" eth_shost,"
								" eth_dhost,"
								" eth_type,"
								" ip_vhl,"
								" ip_tos,"
								" ip_len,"
								" ip_id,"
								" ip_off,"
								" ip_ttl,"
								" ip_p,"
								" ip_sum,"
								" ip_src,"
								" ip_dst from packets", i_import_db_callback, 0, &err_msg);

	if(rc != SQLITE_OK) {
		fprintf(stderr, "Failed to select data\n");
		fprintf(stderr, "SQL error: %s\n", err_msg);

		sqlite3_free(err_msg);
		OCI_Cleanup();
		sqlite3_close(db);

		return 1;
	}

    OCI_Cleanup();
    sqlite3_close(db);

    return EXIT_SUCCESS;
}
