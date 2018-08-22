#include "svc_kernel/svc_lock.h"

#ifndef DEBUG_MODE
KSTATUS lock_init(PLOCKER locker)
#else
KSTATUS lock_init(PLOCKER locker, const char *name)
#endif
{
#ifdef DEBUG_MODE
    locker->_name = name;
    DPRINTF("LOCK_INIT(%s)", locker->_name);
#endif
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&locker->_mutex, &attr);
    return KSTATUS_SUCCESS;
}

void lock_destroy(PLOCKER locker)
{
#ifdef DEBUG_MODE
    DPRINTF("LOCK_DESTROY(%s)", locker->_name);
#endif
    pthread_mutex_destroy(&locker->_mutex);
}

KSTATUS lock(PLOCKER locker)
{
    pthread_mutex_lock(&locker->_mutex);
#ifdef DEBUG_MODE
    DPRINTF("LOCK(%s)", locker->_name);
#endif
    return KSTATUS_SUCCESS;
};

KSTATUS unlock(PLOCKER locker)
{
#ifdef DEBUG_MODE
    DPRINTF("UNLOCK(%s)", locker->_name);
#endif
    pthread_mutex_unlock(&locker->_mutex);
    return KSTATUS_SUCCESS;
};
