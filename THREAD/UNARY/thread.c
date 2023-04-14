#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <sched.h>
#include <setjmp.h>
#include <sys/types.h>
#include "types.h"
#include "lock.h"
#include "queue.h"
#include "stack.h"
#include "thread.h"

static bool started = false;

static struct qthread_t *threads = NULL;
static struct qthread_t *qunused = NULL;
static struct stkattr_t *attributes = NULL;
static struct spinlock_t lock, unqlock, stklock;

/*
   static struct qthread_t *qready = NULL;
   static struct qthread_t *qrunning = NULL;
   static struct qthread_t *qwaiting = NULL;
   static struct qthread_t *qterminated = NULL;
   */

void sigusr_handler (int arg)
{
	fprintf (stderr, "DEBUG (%d) : sigusr_handler : SIGUSR1...\n", gettid ());
	thread_exit (NULL);
	return ;
}

int wrapper (void *arg)
{
	/* exit status obtained by wait() */
	int ret = EXIT_FAILURE;
	struct thread_t *th = NULL;
	struct routine_t *routine = (struct routine_t *) arg;

	if (routine && (th = routine->result))
	{
		ret = EXIT_SUCCESS;

		routine->result = NULL;

		th->state = RUNNING;
		/* returning from routine normally --> 0 */
		/* returning from routine by calling thread_exit() --> 1 */ 

		if (!setjmp (th->context))
		{
			routine->result = routine->fn (routine->arg);
			fprintf (stderr, "DEBUG (%d) : wrapper : normal return from routine...\n", gettid());
		}
		else
			fprintf (stderr, "DEBUG (%d) : wrapper : routine returning from thread_exit()...\n", gettid());

		th->state = TERMINATED;
	}

	return ret;
}

void thread_init ()
{
	/* initialize all the data structures */
	init_spinlock (&lock);
	init_queue (threads);
	init_queue (qunused);
	init_stack (attributes);
	atexit (thread_deinit);

	return ;
}

void thread_deinit ()
{
	/* destroy all the data structures */
	acquire (&lock);
	destroy_queue (threads);
	release (&lock);
	acquire (&unqlock);
	destroy_queue (qunused);
	release (&unqlock);
	acquire (&stklock);
	destroy_stack (attributes);
	release (&stklock);

	return ;
}

int thread_attr_init (struct attr_t *attr)
{
	int ret = -1;

	if (attr && fetch_attribute (&attr))
		ret = 0;

	return ret;
}

int thread_attr_destroy (struct attr_t *attr)
{
	int ret = -1;

	if (attr)
	{
		ret = 0;
		attr = NULL;
	}

	return ret;
}

int thread_create (tid_t *tid, struct attr_t *attr, 
		void *(*start_routine) (void *), void *arg)
{
	int retval = -1, cl_ret = -1;

	struct thread_t *thread = NULL;
	struct routine_t *routine = NULL;

	/* TODO CLONE_SETTLS */
	static int flags = (CLONE_VM | CLONE_FS | CLONE_FILES 
			| CLONE_SIGHAND | CLONE_THREAD | CLONE_PARENT_SETTID 
			| CLONE_CHILD_CLEARTID | CLONE_SYSVSEM);

	if (!started)
	{
		thread_init ();
		started = true;
	}

	if (tid && start_routine)
	{
		if (attr || fetch_attribute (&attr))
		{
			if (fetch_thread (&thread))
			{
				thread->state = EMBRYO;
				if ((routine = (struct routine_t *)
							malloc (sizeof (struct routine_t))))
				{
					routine->fn = start_routine;
					routine->arg = arg;

					/* use result to point to thread_t */
					routine->result = (void *) thread;

					thread->attr = attr;
					thread->routine = routine;
					thread->tid = clone (wrapper, attr->stack + attr->stack_size, 
							flags, (void *) routine);

					if ((*tid = thread->tid) != -1)
					{
						/* clone succeeded */
						retval = 0;
						acquire (&lock);
						enqueue (threads, thread);
						release (&lock);
					}
					else
					{
						/* clone failed, cleanup required */
						clean_routine (&routine);
						clean_attribute (&attr);
						clean_thread (&thread);
						fprintf (stderr, "ERROR :: THREAD_CREATE : CLONE FAILED...\n");
					}
				}
				else
				{
					clean_attribute (&attr);
					clean_thread (&thread);
					fprintf (stderr, "ERROR :: THREAD_CREATE : ROUTINE : MEMORY ALLOCATION FAILED...\n");
				}
			}
			else
			{
				clean_attribute (&attr);
				fprintf (stderr, "ERROR :: THREAD_CREATE : THREAD : MEMORY ALLOCATION FAILED...\n");
			}

		}
		else
		{
			fprintf (stderr, "ERROR :: THREAD_CREATE : ATTRIBUTE : MEMORY ALLOCATION FAILED...\n");
		}

	}
	else
	{
		fprintf (stderr, "ERROR :: THREAD_CREATE : INVALID ARGUMENTS...\n");
	}

	return retval;
}

void thread_exit (void *retval)
{
	tid_t tid = gettid();
	struct thread_t *th = NULL;

	acquire (&lock);
	if ((th = search_thread (threads, tid)))
	{
		th->routine->result = retval;
		release (&lock);
		longjmp (th->context, 1);
	}
	else
		release (&lock);

	return ;
}

int thread_join (tid_t tid, void **retval)
{
	int ret = -1;
	struct thread_t *th = NULL;

	acquire (&lock);
	if ((th = search_thread (threads, tid)))
	{
		release (&lock);
		ret = 0;

		// TODO USE FUTEX TO SLEEP
		while (!(th->state == TERMINATED))
			;

		if (retval)
			*retval = th->routine->result;

		clean_routine (&th->routine);
		clean_attribute (&th->attr);
		clean_thread (&th);
	}
	else
		release (&lock);

	return ret;
}


int thread_cancel (tid_t tid)
{
	int ret = -1;
	struct thread_t *th = NULL;

	acquire (&lock);
	if ((th = search_thread (threads, tid)))
	{
		release (&lock);

		if ((th->state != TERMINATED) && thread_kill (tid, SIGUSR1))
		{
			ret = 0;

			// TODO USE FUTEX TO SLEEP
			while (th->state != TERMINATED)
				;
		}
	}
	else
		release (&lock);

	return ret;
}

int thread_kill (tid_t tid, int sig)
{
	int ret = -1;
	pid_t tgid = getpid ();

	if (!sig || !tgkill (tgid, tid, sig))
		ret = 0;

	return ret;
}

void thread_spin_lock_init (struct spinlock_t *lk)
{
	if (lk)
		init_spinlock (lk);

	return ;
}

int thread_spin_lock (struct spinlock_t *lk)
{
	int ret = -1;

	if (lk)
	{
		ret = 0;
		acquire (lk);
	}

	return ret;
}

int thread_spin_unlock (struct spinlock_t *lk)
{
	int ret = -1;

	if (lk)
	{
		ret = 0;
		release (lk);
	}

	return ret;
}

void thread_mutex_lock_init (struct sleeplock_t *sl)
{
	if (sl)
		init_sleeplock (sl);

	return ;
}

int thread_mutex_lock (struct sleeplock_t *sl)
{
	int ret = -1;

	if (sl)
	{
		ret = 0;
		snooze (sl);
	}

	return ret;
}

int thread_mutex_unlock (struct sleeplock_t *sl)
{
	int ret = -1;

	if (sl)
	{
		ret = 0;
		wake (sl);
	}

	return ret;
}

bool fetch_thread (struct thread_t **th)
{
	bool flag = false;

	if (th)
	{
		acquire (&unqlock);
		*th = dequeue (qunused);
		release (&unqlock);

		if (*th)
			flag = true;
		else if ((*th = (struct thread_t *) 
					calloc (1, sizeof (struct thread_t))))
		{
			flag = true;
			(*th)->tid = -1;
			(*th)->attr = NULL;
			(*th)->routine = NULL;
			(*th)->state = UNUSED;
		}
		else
			fprintf (stderr, "ERROR :: FETCH_THREAD : MEMORY ALLOCATION FAILED...\n");
	}

	return flag;
}

bool clean_thread (struct thread_t **th)
{
	bool flag = false;

	if (th && *th)
	{
		flag = true;

		clean_attribute (&(*th)->attr);
		clean_routine (&(*th)->routine);

		if (qunused->count < BACKUP_THRESHOLD)
		{		
			(*th)->tid = -1;
			(*th)->state = UNUSED;

			acquire (&unqlock);
			enqueue (qunused, *th);
			release (&unqlock);
		}
		else
			free (*th);

		*th = NULL;
	}

	return flag;
}

bool fetch_attribute (struct attr_t **attr)
{
	bool flag = false;

	if (attr)
	{
		acquire (&stklock);
		if (!(*attr = pop (attributes)))
		{
			release (&stklock);
			if ((*attr = (struct attr_t *) calloc (1, sizeof (struct attr_t)))
					&& init_attribute (*attr))
				flag = true;
			else
				*attr ? free (*attr) : fprintf (stderr, 
						"ERROR :: FETCH_ATTRIBUTE : MEMORY ALLOCATION FAILED...\n");
		}
		else
			release (&stklock);
	}

	return flag;
}

bool clean_attribute (struct attr_t **attr)
{
	bool flag = false;

	if (attr && *attr)
	{
		flag = true;

		if (attributes->count < BACKUP_THRESHOLD)
		{
			acquire (&stklock);
			push (attributes, *attr);
			release (&stklock);
		}
		else
			free_attribute (*attr);

		*attr = NULL;
	}

	return flag;
}

bool clean_routine (struct routine_t **routine)
{
	bool flag = false;

	if (routine && *routine)
	{
		flag = true;
		free (*routine);
		*routine = NULL;
	}

	return flag;
}

void free_thread (struct thread_t *th)
{
	if (th)
	{
		free_attribute (th->attr);
		clean_routine (&th->routine);
		free (th);
	}

	return ;
}
