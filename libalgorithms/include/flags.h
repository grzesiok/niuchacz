#ifndef _LIBALGORITHMS_FLAGS_H
#define _LIBALGORITHMS_FLAGS_H
#define flagsCmp(var, flags2) ((var & flags2) == flags2)
#define flagsSet(var, flags) var |= flags
#define flagsUnset(var, flags) var &= ~flags
#endif
