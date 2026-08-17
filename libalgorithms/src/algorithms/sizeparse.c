/* sizeparse.c - implement human-friendly size parsing */
#include <stdlib.h>
#include <ctype.h>
#include "sizeparse.h"

size_t parse_size_string(const char *s, int *ok) {
    if (!s || !s[0]) { if (ok) *ok = 0; return 0; }
    char *endptr = NULL;
    unsigned long long val = strtoull(s, &endptr, 10);
    while (*endptr && isspace((unsigned char)*endptr)) endptr++;
    unsigned long long mul = 1ULL;
    if (*endptr) {
        char u = toupper((unsigned char)*endptr);
        if (u == 'B') { mul = 1ULL; }
        else if (u == 'K') { mul = 1024ULL; }
        else if (u == 'M') { mul = 1024ULL*1024ULL; }
        else if (u == 'G') { mul = 1024ULL*1024ULL*1024ULL; }
        else if (u == 'T') { mul = 1024ULL*1024ULL*1024ULL*1024ULL; }
        else { if (ok) *ok = 0; return 0; }
    }
    if (ok) *ok = 1;
    return (size_t)(val * mul);
}
