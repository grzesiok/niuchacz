#ifndef _SVC_LOCK_H
#define _SVC_LOCK_H
#include "svc_status.h"
#include "../kernel.h"

typedef struct _LOCKER_T
{
	pthread_mutex_t _mutex;
#ifdef DEBUG_MODE
    const char *_name;
#endif
} LOCKER, *PLOCKER;

#define LOCK_VAR LOCKER _locker __attribute__((aligned(CACHE_LINE)))

#ifndef DEBUG_MODE
#define LOCK_INIT(var, type, name) lock_init(&((type*)var)->_locker)
#else
#define LOCK_INIT(var, type, name) lock_init(&((type*)var)->_locker, name)
#endif
#define LOCK_DESTROY(var, type) lock_destroy(&((type*)var)->_locker)

#define LOCK(var, type) lock(&((type*)var)->_locker)
#define UNLOCK(var, type) unlock(&((type*)var)->_locker)

//zwykłe blokady
#ifndef DEBUG_MODE
KSTATUS lock_init(PLOCKER locker);
#else
KSTATUS lock_init(PLOCKER locker, const char *name);
#endif
void lock_destroy(PLOCKER locker);
KSTATUS lock(PLOCKER locker);
KSTATUS unlock(PLOCKER locker);
#endif
