/* sizeparse.h - parse human-friendly size strings */
#ifndef SIZEPARSE_H
#define SIZEPARSE_H

#include <stddef.h>

/* Parse a human-friendly size string like "512MB" or "1G".
 * On success, *ok is set to 1 and the parsed size in bytes is returned.
 * On failure, *ok is set to 0 and 0 is returned.
 */
size_t parse_size_string(const char *s, int *ok);

#endif /* SIZEPARSE_H */
