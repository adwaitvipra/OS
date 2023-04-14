#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/types.h>
#include "types.h"
#include "stack.h"

bool init_attribute (struct attr_t *attr)
{
	bool flag = false;

	/* TODO USE GETRLIMIT() FOR PORTABILITY */
	if (attr)
	{
		if ((attr->stack = mmap (NULL, DEFAULT_STACK_SIZE, PROT_NONE,
				MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK,
				-1, 0)) != MAP_FAILED) 
		{
			if (!mprotect (attr->stack, DEFAULT_STACK_SIZE - DEFAULT_GUARD_SIZE,
						PROT_READ | PROT_WRITE))
			{
				attr->stack_size = DEFAULT_STACK_SIZE - DEFAULT_GUARD_SIZE;
				flag = true;
			}
			else
			{
				free_attribute (attr);
				fprintf (stderr, "ERROR :: INIT_ATTRIBUTE : MEMORY PROTECTION FAILED...\n");
			}

		}
		else
			fprintf (stderr, "ERROR :: INIT_ATTRIBUTE : MEMORY MAPPING FAILED...\n");
	}

	return flag;
}

bool free_attribute (struct attr_t *attr)
{
	bool flag = false;

	if (attr)
	{
		if (!munmap (attr->stack, DEFAULT_STACK_SIZE))
		{
			free (attr);
			flag = true;
		}
		else
			fprintf (stderr, "ERROR :: FREE_ATTRIBUTE : MEMORY UNMAPPING FAILED...\n");
	}

	return flag;
}

bool init_stack (struct stkattr_t *stk)
{
	bool flag = false;

	if ((stk = (struct stkattr_t *) malloc (sizeof (struct stkattr_t))))
	{
		flag = true;

		stk->count = 0;
		stk->top = NULL;
	}

	return flag;
}

bool is_stack_full ()
{
	bool flag = true;
	struct attrnode_t *tmp = NULL;

	if((tmp = (struct attrnode_t *) malloc(sizeof (struct attrnode_t))))
	{
		flag = false;
		free(tmp);
	}

	return flag;
}

bool is_stack_empty (struct stkattr_t *stk)
{
	bool flag = false;

	if(!stk || !stk->count || !stk->top)
		flag = true;

	return flag;
}

void push (struct stkattr_t *stk, struct attr_t *attr)
{
	struct attrnode_t *node = NULL;

	if(stk && attr && !is_stack_full() 
			&& (node = (struct attrnode_t*) 
				malloc (sizeof(struct attrnode_t))))
	{
		stk->count++;
		node->attr = attr;
		node->next = stk->top;
		stk->top = node;
	}

	return;
}

struct attr_t *pop (struct stkattr_t *stk)
{
	struct attr_t *aptr = NULL;
	struct attrnode_t *nptr = NULL;

	if(!is_stack_empty(stk))
	{	
		stk->count--;
		nptr = stk->top;
		aptr = nptr->attr;
		stk->top = nptr->next;
		free (nptr);
	}

	return aptr;
}

struct attr_t *stack_top (struct stkattr_t *stk)
{
	struct attr_t *aptr = NULL;

	if(!is_stack_empty(stk))
		aptr = stk->top->attr;

	return aptr;
}

void display_attribute (struct attr_t *attr)
{
	if (attr)
	{
		fprintf (stderr, "------------------------------------------------------------\n");
		fprintf (stderr, "STACK		: %p\n", attr->stack);
		fprintf (stderr, "STACK_SIZE	: %ld\n", attr->stack_size);
		fprintf (stderr, "------------------------------------------------------------\n");
	}

	return ;
}

void traverse_stack (struct stkattr_t *stk)
{
	struct attrnode_t *iter = NULL;

	if (!is_stack_empty (stk))
	{
		iter = stk->top;
		while(iter)
		{
			if (iter->attr)
				display_attribute (iter->attr);

			iter = iter->next;
		}
	}
	return ;
}

void destroy_stack (struct stkattr_t *stk)
{
	struct attr_t *aptr = NULL;

	if (!is_stack_empty (stk))
	{
		while((aptr = pop (stk)))
			free_attribute (aptr);

		free (stk);
	}

	return ;
}

