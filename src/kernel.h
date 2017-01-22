#ifndef _KERNEL_H
#define _KERNEL_H

#include <stdio.h>
#include <stdlib.h>

#define MALLOC(type, num) (type*)malloc(sizeof(type)*num, 0)
#define REALLOC(var, type, num) (type*)realloc(var, sizeof(type)*num, 0)
#define FREE(var) free(var)
#endif
