#ifndef _SVC_STATUS_H
#define _SVC_STATUS_H
#include "../kernel.h"
typedef unsigned int KSTATUS;

#define KSTATUS_SUCCESS 0
#define KSTATUS_UNSUCCESS 1
#define KSTATUS_OUT_OF_MEMORY 2
#define KSTATUS_SVC_IS_STOPPING 3
#define KSTATUS_DB_OPEN_ERROR 4
#define KSTATUS_DB_EXEC_ERROR 5

#define KSUCCESS(var) (var == KSTATUS_SUCCESS)

#endif
