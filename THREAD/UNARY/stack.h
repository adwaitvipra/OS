#ifndef STACK_H
#define STACK_H

#include <stdbool.h>
#include "types.h"

#define DEFAULT_STACK_SIZE 8392704
#define DEFAULT_GUARD_SIZE 4096

bool init_attribute (struct attr_t *);
bool free_attribute (struct attr_t *);
bool init_stack (struct stkattr_t *);
bool is_stack_full ();
bool is_stack_empty (struct stkattr_t *);
void push (struct stkattr_t *, struct attr_t *);
struct attr_t *pop (struct stkattr_t *);
struct attr_t *stack_top (struct stkattr_t *);
void traverse_stack (struct stkattr_t *);
void destroy_stack (struct stkattr_t *stk);

#endif//STACK_H
