#ifndef _SVC_STATUS_H
#define _SVC_STATUS_H
#include "kernel.h"
typedef unsigned int KSTATUS;

#define KSTATUS_SUCCESS 0
#define KSTATUS_UNSUCCESS 1
#define KSTATUS_OUT_OF_MEMORY 2
#define KSTATUS_SVC_IS_STOPPING 3
#define KSTATUS_DB_MOUNT_ERROR 4
#define KSTATUS_DB_EXEC_ERROR 5
#define KSTATUS_CMDMGR_COMMAND_NOT_FOUND 6
#define KSTATUS_PSMGR_IDLE_MASK_SIGNALS 7
#define KSTATUS_PSMGR_IDLE_WAITPID 8
#define KSTATUS_INVALID_PARAMETERS 9
#define KSTATUS_DB_OPEN_ERROR 10
#define KSTATUS_DB_INCOMPATIBLE_STATE 11

#define KSUCCESS(var) (var == KSTATUS_SUCCESS)

#endif
