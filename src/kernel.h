#ifndef _KERNEL_H
#define _KERNEL_H

#include <syslog.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define DEBUG_MODE

#define MALLOC(type, num) (type*)malloc(sizeof(type)*num)
#define MALLOC2(type, num, extrasize) (type*)malloc(sizeof(type)*num+extrasize)
#define REALLOC(var, type, num) (type*)realloc(var, sizeof(type)*num)
#define FREE(var) free(var)

#ifdef DEBUG_MODE
#define DPRINTF(...) syslog(LOG_DEBUG, __VA_ARGS__)
#define ASSERT(expression) if(!(expression)) {DPRINTF("ASSERT FAIL:%s(%u): %s\n", __FILE__, __LINE__, __FUNCSIG__);}
#else
#define DPRINTF(...)
#define ASSERT(expression)
#endif

//flags operation
typedef unsigned char flags8;
#define CMPFLAGS(var, value) ((var & value) == value)
#define SETFLAGS(var, value) (var |= value)
#define UNSETFLAGS(var, value) (var &= ~value)

#include "compiler.h"
#endif
