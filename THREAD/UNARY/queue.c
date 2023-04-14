#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "queue.h"
#include "types.h"

bool init_queue (struct qthread_t *q)
{
	bool ret = false;

	if ((q = (struct qthread_t *)malloc(sizeof(struct qthread_t))))
	{
		ret = true;
		q->count = 0;
		q->front = q->rear = NULL;
	}

	return ret;
}

bool is_queue_empty (struct qthread_t *q)
{
	bool flag = false;

	if(!q || !q->count || !q->front || !q->rear)
		flag = true;

	return flag;
}

bool is_queue_full ()
{
	bool flag = false;
	struct thnode_t *tmp = NULL;

	if (tmp =(struct thnode_t*)malloc(sizeof(struct thnode_t)))
	{
		flag = true;
		free(tmp);
	}

	return flag;
}

void enqueue (struct qthread_t *q, struct thread_t *th)
{
	struct thnode_t *node = NULL;

	if(q && th && !is_queue_full() 
			&& (node = (struct thnode_t *) 
				malloc (sizeof (struct thnode_t))))
	{
		node->thread = th;
		node->next = NULL;

		q->count++;
		q->rear ? (q->rear->next = node) : (q->front = node);
		q->rear = node;
	}
	return ;
}

struct thread_t *dequeue (struct qthread_t *q)
{
	struct thread_t *ret = NULL;
	struct thnode_t *tmp = NULL;

	if(!is_queue_empty(q))
	{
		ret = q->front->thread;
		tmp = q->front;

		q->front = q->front->next;
		free(tmp);

		q->count--;
	}
	return ret;
}

void display_thread (struct thread_t *thread)
{
	if (thread)
	{
		fprintf (stderr, "------------------------------------------------------------\n");
		fprintf (stderr, "TID		: %d\n", thread->tid);
		fprintf (stderr, "ATTR		: %p\n", thread->attr);

		if (thread->attr)
		{
			fprintf (stderr, "STACK		: %p\n", thread->attr->stack);
			fprintf (stderr, "SIZE 		: %p\n", thread->attr->stack);
		}

		if (thread->routine)
		{
			fprintf (stderr, "FN		: %p\n", thread->routine->fn);
			fprintf (stderr, "ARG		: %p\n", thread->routine->arg);
			fprintf (stderr, "RESULT	: %p\n", thread->routine->result);
		}

		fprintf (stderr, "------------------------------------------------------------\n");
	}
	return ;
}

void traverse_queue (struct qthread_t *q)
{
	struct thnode_t *itr=NULL;

	if(!is_queue_empty(q))
	{
		itr = q->front;
		while(itr)
		{
			if (itr->thread)
				display_thread (itr->thread);

			itr=itr->next;
		}
	}
	return ;
}

struct thnode_t *search_thnode (struct qthread_t *q, tid_t tid)
{
	struct thnode_t *nptr = NULL;

	if (!is_queue_empty(q))
		for (nptr = q->front; nptr && (nptr->thread) && (nptr->thread->tid != tid); nptr = nptr->next)
			;
	return nptr;
}

struct thread_t *search_thread (struct qthread_t *q, tid_t tid)
{
	struct thnode_t *nptr = NULL;

	if (!is_queue_empty(q))
		for (nptr = q->front; nptr && (nptr->thread) && (nptr->thread->tid != tid); nptr = nptr->next)
			;

	return (nptr ? (nptr->thread) : NULL);
}

void destroy_queue (struct qthread_t *q)
{
	struct thread_t *th = NULL;

	if (q)
	{
		while ((th = dequeue (q)))
			free_thread (th);
		free (q);
	}

	return ;
}

void delete_thnode (struct qthread_t *q, tid_t tid)
{
	struct thnode_t *node = NULL;
	struct thread_t *th = NULL;

	if (!is_queue_empty(q) && tid > 0)
	{
		node = search_thnode (q, tid);

		if (node)
		{
			th = rmnode (q, node->thread);
			clean_thread (&th);
		}
	}

	return ;
}

struct thread_t *rmnode (struct qthread_t *q, struct thread_t *th)
{
	struct thread_t *ret = NULL;
	struct thnode_t *xptr = NULL, *yptr = NULL;

	if (th && !is_queue_empty (q))
	{
		xptr = q->front;

		if (q->rear->thread == th)
			ret = dequeue (q);
		else
		{

			while (xptr && xptr->thread != th)
			{
				yptr = xptr;
				xptr = xptr->next;
			}

			if (xptr)
			{

				ret = xptr->thread;

				(xptr == q->front) ? (q->front = xptr->next) 
					: (yptr->next = xptr->next);
				free (xptr);
				q->count--;
			}
		}
	}

	return ret;
}
