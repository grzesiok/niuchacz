#include "spinlock.h"

bool spinlockLockTry(volatile bool* pFlag)
{
   // acquire memory barrier and compiler barrier
   return !__atomic_test_and_set(pFlag, __ATOMIC_ACQUIRE);
}

void spinlockLock(volatile bool* pFlag)
{
    for(;;) {
        // acquire memory barrier and compiler barrier
        if (!__atomic_test_and_set(pFlag, __ATOMIC_ACQUIRE)) {
            return;
        }

        // relaxed waiting, usually no memory barriers (optional)
        while(__atomic_load_n(pFlag, __ATOMIC_RELAXED)) {
            asm("pause");
        }
    }
}

void spinlockUnlock(volatile bool* pFlag)
{
    // release memory barrier and compiler barrier
    __atomic_clear(pFlag, __ATOMIC_RELEASE);
}
