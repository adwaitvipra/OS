#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include "types.h"

bool init_queue (struct qthread_t *);
bool is_queue_full ();
bool is_queue_empty (struct qthread_t *);
void enqueue (struct qthread_t *, struct thread_t *);
struct thread_t *dequeue (struct qthread_t *);
void traverse (struct qthread_t *);
struct thnode_t *search_thnode (struct qthread_t *, tid_t);
struct thread_t *search_thread (struct qthread_t *, tid_t);
void delete_thnode (struct qthread_t *q, tid_t tid);
struct thread_t *rmnode (struct qthread_t *, struct thread_t *);
void destroy_queue (struct qthread_t *);
void traverse_queue (struct qthread_t *);

extern void free_thread (struct thread_t *);
extern bool clean_thread (struct thread_t **);
#endif//QUEUE_H
