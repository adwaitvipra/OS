#define _GNU_SOURCE
#include <stdio.h> 
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include "types.h"
#include "lock.h"

void asleep (void)
{
	return ;
}

void awaken (void)
{
	return ;
}

static inline uint_t xchg (volatile uint_t *address, 
		uint_t value)
{
	uint_t result;

	asm volatile (
			"lock; xchgl %0, %1" :
			"+m" (*address), "=a" (result) :
			"1" (value) :
			"cc"
		     );

	return result;
}

void init_spinlock (struct spinlock_t *lk)
{
	if (lk)
	{
		lk->locked = 0;
		lk->tid = -1;
	}

	return ;
}

bool holding (struct spinlock_t *lk)
{
	bool flag = false;

	if (lk)
		flag = (bool) (lk->locked && (lk->tid == gettid()));

	return flag;
}

void acquire (struct spinlock_t *lk)
{
	if (lk)
	{
		if (holding (lk))
		{
			fprintf (stderr, "acquire : lock already acquired\n");
			exit (1);
		}

		// xchg is atomic due to lock
		while (xchg (&lk->locked, 1))
			;

		lk->tid = gettid();
	}

	return ;
}

void release (struct spinlock_t *lk)
{
	if (lk)
	{
		if (!holding (lk))
		{
			fprintf (stderr, "release : lock not acquired\n");
			exit(1);
		}

		asm volatile ("movl $0, %0" : "+m" (lk->locked) : );

	}

	return ;
}

void init_sleeplock (struct sleeplock_t *sl)
{
	if (sl)
	{
		init_spinlock (&sl->spinlk);
		sl->locked = 0;
		sl->tid = -1;
	}

	return ;
}

bool snoozed (struct sleeplock_t *sl)
{
	bool flag = false;

	if (sl)
	{
		acquire (&sl->spinlk);
		flag = sl->locked && (sl->tid == gettid());
		release (&sl->spinlk);
	}

	return flag;
}

void snooze (struct sleeplock_t *sl)
{
	if (sl)
	{
		acquire (&sl->spinlk);
		
		if (snoozed (sl))
		{
			fprintf (stderr, "snooze : already snoozed\n");
			release (&sl->spinlk);
			exit (1);
		}

		while (sl->locked)
		{
			release (&sl->spinlk);

			/* call to futex for sleep */
			asleep ();

			acquire (&sl->spinlk);
		}

		sl->locked = 1;
		sl->tid = gettid();

		release (&sl->spinlk);
	}

	return ;
}

void wake (struct sleeplock_t *sl)
{
	if (sl)
	{
		acquire (&sl->spinlk);

		if (!snoozed (sl))
		{
			fprintf (stderr, "wake : not snoozed\n");
			release (&sl->spinlk);
			exit (1);
		}

		sl->locked = 0;
		sl->tid = -1;

		/* call to futex for wakeup */
		awaken ();

		release (&sl->spinlk);
	}

	return ;
}
