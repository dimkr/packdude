#ifndef _STACK_H_INCLUDED
#	define _STACK_H_INCLUDED

#	include <stdbool.h>

#	include "result.h"

/*!
 * @defgroup stack Stack
 * @brief A FIFO data structure
 * @{ */

typedef struct node node_t;

/*!
 * @struct node
 * @brief A stack node */
struct node {
	const void *data; /*!< The node payload */
	node_t *next; /*!< The next node */
};

/*!
 * @typedef node_comparison_t
 * @brief A callback used to compare two nodes
 *
 * This function should return 0 when \a a and \a b are identical. */
typedef int (*node_comparison_t)(const void *a, const void *b);

/*!
 * @fn result_t stack_push(node_t **stack, const void *data)
 * @brief Appends data to a stack
 * @param stack A stack
 * @param data The data
 * @see stack_pop */
result_t stack_push(node_t **stack, const void *data);

/*!
 * @fn void stack_pop(node_t **stack)
 * @brief Removes the first node from a stack
 * @param stack A stack
 * @see stack_push */
void stack_pop(node_t **stack);

/*!
 * @fn bool stack_contains(const node_t *stack,
 *                         const node_comparison_t callback,
 *                         const void *data);
 * @brief Determines whether a given payload is part of a stack
 * @param stack A stack
 * @param callback A comparison callback
 * @param data The payload to search for */
bool stack_contains(const node_t *stack,
                    const node_comparison_t callback,
                    const void *data);

/*!
 * @} */

#endif
