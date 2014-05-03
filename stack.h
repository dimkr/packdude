#ifndef _STACK_H_INCLUDED
#	define _STACK_H_INCLUDED

#	include <stdbool.h>
#	include "result.h"

typedef struct node node_t;

struct node {
	const void *data;
	node_t *next;
};

typedef int (*node_comparison_t)(const void *a, const void *b);

result_t stack_push(node_t **stack, const void *data);
void stack_pop(node_t **stack);

bool stack_contains(const node_t *stack,
                    const node_comparison_t callback,
                    const void *data);

#endif