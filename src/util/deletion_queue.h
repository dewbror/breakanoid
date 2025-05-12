/*
  deletion_queue.h
*/

#ifndef DELETION_QUEUE_H_
#define DELETION_QUEUE_H_
#pragma once

#include <stdbool.h>

/**
 * A deletion node, holds a pointer to the resource which is to be deleted, a pointer to the function which will deletes
 * the resource and a pointer to the previous node in the queue.
 */
typedef struct deletion_node {
    void* p_resource;
    void (*delete_func)(void*);
    struct deletion_node* p_prev;
} deletion_node;

/**
 * A deletion queue, holds pointers to the first and last nodes in the queue.
 */
typedef struct deletion_queue {
    deletion_node* p_first;
    deletion_node* p_last;
} deletion_queue;

/**
 * Allocate a new deletion_queue on the heap, the deletion queue is deleted when using deletion_queue_flush.
 *
 * @return Pointer to newly allocated deletion_queue.
 */
deletion_queue* deletion_queue_alloc(void);

/**
 * Allocate a new deletion node on the heap and add it to the end of the deletion queue.
 *
 * @param[in] p_queue       Pointer to the deletion queue the new node is added to.
 * @param[in] p_resource    Pointer to the resource to be deleted by the deletion function.
 * @param[in] deletion_func Pointer to the function that will delete the resource.
 *
 * @returns True if successful, false if failed.
 */
bool deletion_queue_queue(deletion_queue* p_queue, void* p_resource, void (*deletion_func)(void*));

/**
 * Flush the deletion queue. Callbacks the deletion functions in the deletion queue from last to first. *pp_queue is
 * freed and set to NULL
 *
 * \param[in] pp_queue Double pointer to the deletion_queue to flush.
 *
 * \return True if successful, false if failed.
 */
bool deletion_queue_flush(deletion_queue** pp_queue);
#endif // DELETION_QUEUE_H_
