#ifndef DELETION_QUEUE_H_
#define DELETION_QUEUE_H_
#pragma once

/**
 * deletion_queue.h
 */

// Deletion Node
typedef struct deletion_node deletion_node;
struct deletion_node {
    void *p_resource;
    void(*delete_func)(void *);
    deletion_node *p_prev;
};

// Deletion Queue
typedef struct {
    deletion_node *p_first;
    deletion_node *p_last;
} deletion_queue;

deletion_queue *deletion_queue_alloc(void);
bool deletion_queue_queue(deletion_queue *queue, void *resource, void (*deletion_func)(void *));
void deletion_queue_flush(deletion_queue *queue);
#endif // DELETION_QUEUE_H_
