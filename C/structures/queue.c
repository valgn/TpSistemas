#include "queue.h"
#include <stdio.h>
#include <stdlib.h>


/**
 * Creates a new empty queue.
 */
Queue *create_queue()
{
    Queue *queue = malloc(sizeof(Queue));
    queue->first = NULL;
    queue->last = NULL;
    return queue;
}

/**
 * Adds an element to the end of the queue.
 */
void enqueue(Queue *queue, void* data, CopyFunction copy )
{
    // Create the new node to be appended.
    Nodo *nodo = malloc(sizeof(Nodo));
    nodo->data = copy(data);
    nodo->next = NULL;
    if(queue->first == NULL)
    {
        // Queue was empty: the new node becomes both first and last.
        queue->first = nodo;
        queue->last= nodo;
    }
    else 
    {
        // Queue was not empty: append after the current last node.
        queue->first->next = nodo;
        queue->last = nodo;
    }
}

/**
 * Removes and returns the data at the front of the queue.
 * Returns -1 if the queue is empty.
 */
void* dequeue(Queue *queue, CopyFunction copy, DestroyFunction destroy )
{
    if (queue->first != NULL)
    {
        // Save the next node before freeing the current first node.
        Nodo *new_first = queue->first->next;
        int dequeued_data = copy(queue->first->data);
        destroy(queue->first->data);
        free(queue->first);
        queue->first = new_first;
        return dequeued_data;
    }

    return -1;
}

/**
 * Destroys the queue, freeing every remaining node and applying the
 * destroy function to each dequeued data.
 */
void destroy_queue(Queue *queue ,DestroyFunction destroy )
{
    while(queue->first != NULL)
      destroy(dequeue(queue));
    free(queue);
}