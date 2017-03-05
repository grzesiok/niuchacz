#ifndef _KERNEL_H
#define _KERNEL_H

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
#define DPRINTF(...) printf(__VA_ARGS__)
//printf("<%u>:"##msg, gettid(), __VA_ARGS__)
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
