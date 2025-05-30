#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logger.h"
#include "error/error.h"

#include "util/deletion_stack.h"

deletion_stack_t* deletion_stack_init(void)
{
    // Allocate new deletion stack
    deletion_stack_t* p_queue = (deletion_stack_t*)malloc(sizeof(deletion_stack_t));
    if(p_queue == NULL) {
        LOG_ERROR("%s: Failed to allocate memory of size: %lu", __func__, sizeof(deletion_stack_t));
        return NULL;
    }

    p_queue->p_first = NULL;
    p_queue->p_last = NULL;

    LOG_DEBUG("%s: init successful", __func__);

    return p_queue;
}

error_t deletion_stack_push(deletion_stack_t* p_stack, void* p_resource, void (*delete_func)(void*))
{
    if(p_stack == NULL)
        return error_init(ERR_SRC_CORE, ERR_NULL_ARG, "%s: p_stack is NULL", __func__);

    // Allocate memory for new deletion_node
    deletion_node_t* p_new_node = (deletion_node_t*)malloc(sizeof(deletion_node_t));
    if(p_new_node == NULL)
        return error_init(ERR_SRC_CORE, ERR_MALLOC, "%s: Failed to allocate memory of size %lu", __func__,
            sizeof(deletion_node_t));

    // Set the new deletion_node fields
    p_new_node->p_resource = p_resource;
    p_new_node->p_prev = NULL;
    p_new_node->delete_func = delete_func;

    // Make sure the new node points to the previous node
    if(p_stack->p_last != NULL) {
        p_new_node->p_prev = p_stack->p_last;
    }
    else {
        p_stack->p_first = p_new_node;
    }

    p_stack->p_last = p_new_node;

    LOG_DEBUG("Push on deletion stack successful");

    return SUCCESS;
}

error_t deletion_stack_flush(deletion_stack_t** pp_queue)
{
    // Check if pp_queue is NULL
    if(pp_queue == NULL)
        return error_init(ERR_SRC_CORE, ERR_NULL_ARG, "%s: pp_queue is NULL", __func__);

    // Dereference pointer
    deletion_stack_t* p_queue = *pp_queue;

    // Check if *pp_queue is NULL
    if(p_queue == NULL)
        return error_init(ERR_SRC_CORE, ERR_NULL_ARG, "%s: p_queue is NULL", __func__);

    // Flush deletion stack
    LOG_DEBUG("Flushing deletion stack");
    while(p_queue->p_last != NULL) {
        deletion_node_t* p_node = p_queue->p_last;
        p_queue->p_last = p_node->p_prev;

        // Call delete function on resource
        if(p_node->delete_func != NULL) {
            p_node->delete_func(p_node->p_resource);
        }

        // Free the deletion node
        free(p_node);
        p_node = NULL;
    }

    // Free the deletion stack itself and set it to NULL
    free(p_queue);
    *pp_queue = NULL;

    LOG_DEBUG("Deletion stack flushed");

    return SUCCESS;
}
