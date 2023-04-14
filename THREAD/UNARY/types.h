#ifndef TYPES_H
#define TYPES_H

#include <sys/types.h>
#include <setjmp.h>
#define BACKUP_THRESHOLD 128

typedef unsigned int uint_t;

/* thread */

enum thstate {UNUSED, EMBRYO, READY, RUNNING, WAITING, TERMINATED};

typedef pid_t tid_t;

/*
 * stores tid (system-wide, as returned by gettid()) as primary key
 * state of thread to take decisions while processing
 * pointers to attributes and routine belonging to thread
 * reusable, released on join 
 */
struct thread_t 
{
	tid_t tid;
	enum thstate state;
	struct attr_t *attr;
	struct routine_t *routine;
	jmp_buf context;
};

/*
 * reusable and released on exit
 */
struct attr_t
{
	void *stack;
	size_t stack_size;
};

/* 
 * stores information related to function, passed to wrapper function as argument
 * retains result after exit, result points to tcb before execution
 * released on join, not reusable
 */
struct routine_t
{
	void *(*fn) (void *);
	void *arg;
	void *result; // when unused points to tcb
};

/* lock */
typedef uint_t lock_t;

struct spinlock_t
{
	tid_t tid;
	lock_t locked;
};

struct sleeplock_t
{
	tid_t tid;
	lock_t locked;
	struct spinlock_t spinlk;
};

/* stack */
struct attrnode_t
{
	struct attr_t *attr;
	struct attrnode_t *next;
};

struct stkattr_t
{
	uint_t count;
	struct attrnode_t *top;
};

/* queue */
struct thnode_t
{
	struct thread_t *thread;
	struct thnode_t *next;
};

struct qthread_t
{
	uint_t count;
	struct thnode_t *front;
	struct thnode_t *rear;
};
#endif//TYPES_H
