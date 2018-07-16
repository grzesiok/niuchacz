#ifndef _IMPORT_H
#define _IMPORT_H
#include <sys/time.h>

typedef void (*import_callback_t)(const unsigned char *packet, struct timeval ts, unsigned int packet_len);

int import_file(const char* pfile_name, import_callback_t *pcallback_list, int list_size);
#endif
