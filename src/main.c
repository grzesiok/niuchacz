#include "kernel.h"
#include "database.h"
#include "import.h"
#include <openssl/md5.h>

const char *to_str(struct timeval ts) {
	static char timestamp_string_buf[64];
	sprintf(timestamp_string_buf, "%d.%06d", (int) ts.tv_sec, (int) ts.tv_usec);
	return timestamp_string_buf;
}

void frames_callback(const unsigned char *packet, struct timeval ts, unsigned int packet_len) {
	MD5_CTX c;
	unsigned char packet_md5[MD5_DIGEST_LENGTH];
	unsigned char stmt[512];

	MD5_Init(&c);
    MD5_Update(&c, packet, packet_len);
    MD5_Final(packet_md5, &c);
    sprintf(stmt, "insert into frames (ts, len, hash) "
    			  "values (%s, %d, %s, %d)", to_str(ts), packet_len, packet_md5);
    sql_stmt(stmt);
}

int main(int argc, char* argv[])
{
	int  rc;

	/* Open database */
	rc = sqlite3_open(argv[1], &db);
	if (rc) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		return 0;
	} else {
		fprintf(stdout, "Opened database successfully\n");
	}
	import_callback_t callback_list[1];

	import_file(argv[2], callback_list, sizeof(callback_list)/sizeof(import_callback_t));

	sqlite3_close(db);
	return 0;
}
