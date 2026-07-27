#ifndef _LIBALGORITHMS_ALGORITHMS_SPINLOCK_H
#define _LIBALGORITHMS_ALGORITHMS_SPINLOCK_H

#include <stdbool.h>

/* 
 * Spinlock state flag type.
 * Must be a 1-byte unsigned character to satisfy compiler atomic builtins.
 */
typedef unsigned char spinlock_t;

/* Standard spinlock initializers */
#define SPINLOCK_UNLOCKED 0
#define SPINLOCK_LOCKED   1

/* Public API Function Prototypes */
bool spinlockLockTry(spinlock_t *pFlag);
void spinlockLock(spinlock_t *pFlag);
void spinlockUnlock(spinlock_t *pFlag);

#endif /* _LIBALGORITHMS_ALGORITHMS_SPINLOCK_H */