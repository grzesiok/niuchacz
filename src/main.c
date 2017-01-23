#include "kernel.h"
#include "database.h"
#include "import.h"
#include <openssl/md5.h>

void frames_callback(const unsigned char *packet, struct timeval ts, unsigned int packet_len) {
	MD5_CTX c;
	unsigned char packet_md5[MD5_DIGEST_LENGTH];
	unsigned char stmt[512];

	MD5_Init(&c);
    MD5_Update(&c, packet, packet_len);
    MD5_Final(packet_md5, &c);
    sprintf(stmt, "insert into frames (ts, len, hash) "
    			  "values (%d.%06d, %d, \"%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\")",
				  (int) ts.tv_sec, (int) ts.tv_usec, packet_len,
				  packet_md5[0], packet_md5[1], packet_md5[2], packet_md5[3],
				  packet_md5[4], packet_md5[5], packet_md5[6], packet_md5[7],
				  packet_md5[8], packet_md5[9], packet_md5[10], packet_md5[11],
				  packet_md5[12], packet_md5[13], packet_md5[14], packet_md5[15]);
    printf("%s\n", stmt);//sql_stmt(stmt);
}

int main(int argc, char* argv[])
{
	int  rc;

	printf("Importing file=%s into db=%s\n", argv[2], argv[1]);

	/* Open database */
	/*rc = sqlite3_open(argv[1], &db);
	if (rc) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		return 0;
	} else {
		fprintf(stdout, "Opened database successfully\n");
	}*/
	import_callback_t callback_list[] = {frames_callback};

	import_file(argv[2], callback_list, sizeof(callback_list)/sizeof(import_callback_t));

	/*sqlite3_close(db);*/
	return 0;
}
