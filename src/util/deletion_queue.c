/*
  deletion_queue.c
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/deletion_queue.h"
#include "logger.h"

deletion_queue* deletion_queue_alloc(void) {
    deletion_queue* p_queue = (deletion_queue*)malloc(sizeof(deletion_queue));
    // TODO: Create my own malloc
    if(!p_queue) {
        LOG_ERROR("Failed to allocate deletion queue: %s", strerror(errno));
        return NULL;
    }
    // Memset not necessary since we are setting the field members directly after allocation
    // memset(p_queue, 0, sizeof(deletion_queue));
    p_queue->p_first = NULL;
    p_queue->p_last  = NULL;

    LOG_TRACE("Deletion queue allocated successfully");
    return p_queue;
}

bool deletion_queue_queue(deletion_queue* p_queue, void* p_resource, void (*delete_func)(void*)) {
    // Allocate memory for new deletion_node
    deletion_node* p_new_node = (deletion_node*)malloc(sizeof(deletion_node));
    // TODO: Create my own malloc that does allocation checking and zero init.
    if(!p_new_node) {
        // handle malloc error
        LOG_ERROR("Failed to allocate deletion node: %s", strerror(errno));
        return false;
    }
    // memset(p_new_node, 0, sizeof(deletion_node));
    // Set the new deletion_node fields
    p_new_node->p_resource  = p_resource;
    p_new_node->p_prev      = NULL;
    p_new_node->delete_func = delete_func;
    
    // Make sure the new node points to the previous node
    if(p_queue->p_last) {
        p_new_node->p_prev = p_queue->p_last;
    } else {
        p_queue->p_first = p_new_node;
    }
    p_queue->p_last = p_new_node;

    LOG_TRACE("Deletion node allocated successfully");
    return true;
}

void deletion_queue_flush(deletion_queue* p_queue) {
    // Check if p_queue is NULL
    if(!p_queue) {
        LOG_WARN("Deletion queue is NULL");
        return;
    }
    LOG_TRACE("Flushing deletion queue");
    while(p_queue->p_last) {
        deletion_node* p_node = p_queue->p_last;
        p_queue->p_last       = p_node->p_prev;

        // Call delete function on resource
        if(p_node->delete_func)
            p_node->delete_func(p_node->p_resource);

        // Free the deletion node
        free(p_node);
    }
    // Free the deletion queue itself
    free(p_queue);
    LOG_TRACE("Deletion queue flushed");
}
