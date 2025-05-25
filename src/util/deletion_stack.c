#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/deletion_stack.h"
#include "logger.h"

deletion_stack_t* deletion_stack_init(void) {
    // Allocate new deletion queue
    deletion_stack_t* p_queue = (deletion_stack_t*)malloc(sizeof(deletion_stack_t));
    if(p_queue == NULL) {
        LOG_ERROR("Failed to initiate deletion stack: %s", strerror(errno));
        return NULL;
    }

    p_queue->p_first = NULL;
    p_queue->p_last  = NULL;

    LOG_DEBUG("Deletion stack initiated successfully");
    return p_queue;
}

bool deletion_stack_push(deletion_stack_t* p_queue, void* p_resource, void (*delete_func)(void*)) {
    // Allocate memory for new deletion_node
    deletion_node_t* p_new_node = (deletion_node_t*)malloc(sizeof(deletion_node_t));
    if(p_new_node == NULL) {
        // Handle malloc error
        LOG_ERROR("Failed to push on deletion stack: %s", strerror(errno));
        return false;
    }

    // memset(p_new_node, 0, sizeof(deletion_node));
    // Set the new deletion_node fields
    p_new_node->p_resource  = p_resource;
    p_new_node->p_prev      = NULL;
    p_new_node->delete_func = delete_func;

    // Make sure the new node points to the previous node
    if(p_queue->p_last != NULL) {
        p_new_node->p_prev = p_queue->p_last;
    } else {
        p_queue->p_first = p_new_node;
    }
    p_queue->p_last = p_new_node;

    LOG_DEBUG("Push on deletion stack successfully");
    return true;
}

bool deletion_stack_flush(deletion_stack_t** pp_queue) {
    // Check if pp_queue is NULL
    if(pp_queue == NULL) {
        LOG_ERROR("deletion_stack_flush: pp_queue is NULL");
        return false;
    }

    // Dereference pointer
    deletion_stack_t* p_queue = *pp_queue;

    // Check if *pp_queue is NULL
    if(p_queue == NULL) {
        LOG_ERROR("deletion_stack_flush: *pp_queue is NULL");
        return false;
    }

    // Flush deletion queue
    LOG_DEBUG("Flushing deletion stack");
    while(p_queue->p_last != NULL) {
        deletion_node_t* p_node = p_queue->p_last;
        p_queue->p_last         = p_node->p_prev;

        // Call delete function on resource
        if(p_node->delete_func != NULL) {
            p_node->delete_func(p_node->p_resource);
        }

        // Free the deletion node
        free(p_node);
        p_node = NULL;
    }

    // Free the deletion queue itself and set it to NULL
    free(p_queue);
    *pp_queue = NULL;
    LOG_DEBUG("Deletion stack flushed");
    return true;
}
