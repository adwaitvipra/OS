#ifndef LOCK_H
#define LOCK_H

#include "types.h"

void asleep (void);
void awaken (void);

void init_spinlock (struct spinlock_t *);
bool holding (struct spinlock_t *);
void acquire (struct spinlock_t *);
void release (struct spinlock_t *);

void init_sleeplock (struct sleeplock_t *);
bool snoozed (struct sleeplock_t *);
void snooze (struct sleeplock_t *);
void wake (struct sleeplock_t *);

#endif//LOCK_H
