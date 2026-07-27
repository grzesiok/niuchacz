#include "spinlock.h"

bool spinlockLockTry(spinlock_t *pFlag)
{
    /* Acquire memory barrier and compiler barrier */
    return !__atomic_test_and_set(pFlag, __ATOMIC_ACQUIRE);
}

void spinlockLock(spinlock_t *pFlag)
{
    for (;;) {
        /* Test-and-Set: Try to capture the lock immediately */
        if (!__atomic_test_and_set(pFlag, __ATOMIC_ACQUIRE)) {
            return;
        }

        /* Test: Relaxed spinning loop to avoid memory bus flooding */
        while (__atomic_load_n(pFlag, __ATOMIC_RELAXED) == SPINLOCK_LOCKED) {
            #if defined(__x86_64__) || defined(_M_X64)
            asm volatile("pause");
            #endif
        }
    }
}

void spinlockUnlock(spinlock_t *pFlag)
{
    /* Release memory barrier and compiler barrier */
    __atomic_clear(pFlag, __ATOMIC_RELEASE);
}