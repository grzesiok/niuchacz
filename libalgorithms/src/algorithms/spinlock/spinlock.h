#ifndef _LIBALGORITHMS_ALGORITHMS_SPINLOCK_H
#define _LIBALGORITHMS_ALGORITHMS_SPINLOCK_H
#include <stdbool.h>

bool spinlockLockTry(volatile bool* pFlag);
void spinlockLock(volatile bool* pFlag);
void spinlockUnlock(volatile bool* pFlag);

#endif
