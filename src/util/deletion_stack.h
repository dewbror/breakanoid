#ifndef DELETION_STACK_H_
#define DELETION_STACK_H_

#include "error/error.h"

/**
 * A deletion node, holds a pointer to the resource which is to be deleted, a pointer to the function which will deletes
 * the resource and a pointer to the previous node in the queue.
 */
typedef struct deletion_node_s {
    void* p_resource;
    void (*delete_func)(void*);
    struct deletion_node_s* p_prev;
} deletion_node_t;

/**
 * A deletion queue, holds pointers to the first and last nodes in the queue.
 */
typedef struct deletion_stack_s {
    deletion_node_t* p_first;
    deletion_node_t* p_last;
} deletion_stack_t;

/**
 * Allocate a new deletion stack on the heap, the deletion stack is freed when using deletion_stack_flush.
 *
 * \return Pointer to newly allocated deletion stack.
 */
deletion_stack_t* deletion_stack_init(void);

/**
 * Allocate a new deletion node on the heap and push it onto the deletion stack.
 *
 * \param[in] p_queue Pointer to the deletion stack the new node is pushed to.
 * \param[in] p_resource Pointer to the resource to be deleted by the deletion function.
 * \param[in] deletion_func Pointer to the function that will delete the resource.
 * \returns True if successful, false if failed.
 */
error_t deletion_stack_push(deletion_stack_t* p_stack, void* p_resource, void (*deletion_func)(void*));

/**
 * Flush the deletion stack. Callbacks the deletion functions in the deletion stack from last to first. *pp_queue is
 * freed and set to NULL
 *
 * \param[in] pp_queue Double pointer to the deletion stack to flush.
 * \return True if successful, false if failed.
 */
error_t deletion_stack_flush(deletion_stack_t** pp_stack);

#endif // DELETION_STACK_H_
