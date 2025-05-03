/*
  deletion_queue.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/deletion_queue.h"

deletion_queue* deletion_queue_alloc(void) {
    deletion_queue* p_queue = (deletion_queue*)malloc(sizeof(deletion_queue));
    // TODO: Create my own malloc
    if(!p_queue) {
        printf("Failed to allocate deletion queue.");
        return NULL;
    }
    // Memset not necessary since we are setting the field members directly after allocation
    // memset(p_queue, 0, sizeof(deletion_queue));
    p_queue->p_first = NULL;
    p_queue->p_last  = NULL;
    printf("Deletion queue allocated\n");
    return p_queue;
}

bool deletion_queue_queue(deletion_queue* p_queue, void* p_resource, void (*delete_func)(void*)) {
    deletion_node* p_new_node = (deletion_node*)malloc(sizeof(deletion_node));
    // TODO: Create my own malloc that does allocation checking and zero init.
    if(!p_new_node) {
        printf("Failed to allocate deletion node\n");
        return false;
    }
    // memset(p_new_node, 0, sizeof(deletion_node));
    p_new_node->p_resource  = p_resource;
    p_new_node->p_prev      = NULL;
    p_new_node->delete_func = delete_func;

    if(p_queue->p_last) {
        p_new_node->p_prev = p_queue->p_last;
    } else {
        p_queue->p_first = p_new_node;
    }
    p_queue->p_last = p_new_node;
    return true;
}

void deletion_queue_flush(deletion_queue* p_queue) {
    if(!p_queue) {
        printf("Deletion queue is NULL\n");
        return;
    }
    printf("\nFlushing deletion queue\n");
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
    printf("Deletion queue flushed\n");
}
