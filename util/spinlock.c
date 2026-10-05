#include "util/spinlock.h"

void spinlock_init(spinlock_t *lock) {
    lock->s_locked = 0;
}

void spinlock_lock(spinlock_t *lock) {
    while (1) {
        while (lock->s_locked != 0) {}; // spin!
        if (__sync_val_compare_and_swap(&lock->s_locked, 0, 1) == 0) {
            return;
        }
    }
}

void spinlock_unlock(spinlock_t *lock) {
    if (__sync_val_compare_and_swap(&lock->s_locked, 1, 0) != 1) {
        // TODO: error!
    }
}
