#ifndef THREAD_H
#define THREAD_H

#include "types.h"
void thread_init ();
void thread_deinit ();
int thread_attr_init (struct attr_t *);
int thread_attr_destroy (struct attr_t *);

int thread_create (tid_t *, struct attr_t *, void *(*) (void *), void *);
int thread_join (tid_t, void **);
void thread_exit (void *);
int thread_cancel (tid_t);
int thread_kill (tid_t, int);
void thread_spin_lock_init (struct spinlock_t *);
int thread_spin_lock (struct spinlock_t *);
int thread_spin_unlock (struct spinlock_t *);
void thread_mutex_lock_init (struct sleeplock_t *);
int thread_mutex_lock (struct sleeplock_t *);
int thread_mutex_unlock (struct sleeplock_t *);

void sigusr_handler (int);
int wrapper (void *);

bool fetch_thread (struct thread_t **);
bool clean_thread (struct thread_t **);
void free_thread (struct thread_t *);
bool fetch_attribute (struct attr_t **);
bool clean_attribute (struct attr_t **);
bool clean_routine (struct routine_t **);

#endif//THREAD_H
