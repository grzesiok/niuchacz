#ifndef _KERNEL_H
#define _KERNEL_H

#include <syslog.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MALLOC(type, num) (type*)malloc(sizeof(type)*num)
#define MALLOC2(type, num, extrasize) (type*)malloc(sizeof(type)*num+extrasize)
#define REALLOC(var, type, num) (type*)realloc(var, sizeof(type)*num)
#define FREE(var) free(var)

#define memoryPtrMove(ptr, diff) (void*)((unsigned long long)ptr+diff)

#ifdef DEBUG_MODE
#define SYSLOG(mode, format, ...) printf("[%s:%d]: " # format "\n", /*__FILE__, */__FUNCTION__, __LINE__, ##__VA_ARGS__)
#define DPRINTF(...) SYSLOG(LOG_DEBUG, __VA_ARGS__)
#define ASSERT(expression) if(!(expression)) {DPRINTF("ASSERT FAIL:%s(%u): %s\n", __FILE__, __LINE__, __FUNCSIG__);}
#else
#define SYSLOG(mode, format, ...) syslog(mode, format, ##__VA_ARGS__)
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
