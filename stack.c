#include <stdlib.h>
#include <assert.h>

#include "stack.h"

node_t *_get_previous(node_t *node, node_t *end) {
	node_t *last = node;

	if (NULL != last) {
		for ( ; end != last->next; last = last->next);
	}
	return last;
}

node_t *_get_last(node_t *node) {
	return _get_previous(node, NULL);
}

node_t *_get_before_last(node_t *node) {
	return _get_previous(node, _get_previous(node, NULL));
}

result_t stack_push(node_t **stack, const void *data) {
	/* the new node */
	node_t *new = NULL;

	/* the return value */
	result_t result = RESULT_MEM_ERROR;

	assert(NULL != stack);
	assert(NULL != data);

	/* allocate memory for the new node */
	new = malloc(sizeof(node_t));
	if (NULL == new) {
		goto end;
	}

	/* push the node to the stack */
	new->data = data;
	new->next = NULL;
	if (NULL == *stack) {
		*stack = new;
	} else {
		_get_last(*stack)->next = new;
	}

	/* report success */
	result = RESULT_OK;

end:
	return result;
}

void stack_pop(node_t **stack) {
	/* the node before the last one */
	node_t *before_last = NULL;

	/* if the stack has only one node, empty the stack */
	if (NULL == (*stack)->next) {
		free(*stack);
		*stack = NULL;
	} else {
		/* detach the last node */
		before_last = _get_before_last(*stack);
		if (NULL != before_last) {
			free(before_last->next);
			before_last->next = NULL;
		}
	}
}

bool stack_contains(const node_t *stack,
                    const node_comparison_t callback,
                    const void *data) {
	/* a node */
	const node_t *node = stack;

	/* the return value */
	bool found = false;

	assert(NULL != callback);
	assert(NULL != data);

	for ( ; NULL != node; node = node->next) {
		if (0 == callback(node->data, data)) {
			found = true;
			break;
		}
	}

	return found;
}