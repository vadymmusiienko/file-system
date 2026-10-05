#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

// lock stuff
typedef struct spinlock {
    volatile char s_locked;
} spinlock_t;

void spinlock_init(spinlock_t *lock);

void spinlock_lock(spinlock_t *lock);

void spinlock_unlock(spinlock_t *lock);

#endif // __SPINLOCK_H__
