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
 * Adds an element with the given id to the end of the queue.
 */
void enqueue(Queue *queue, int id)
{
    // Create the new node to be appended.
    Nodo *nodo = malloc(sizeof(Nodo));
    nodo->id = id;
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
 * Removes and returns the id at the front of the queue.
 * Returns -1 if the queue is empty.
 */
void* dequeue(Queue *queue)
{
    if (queue->first != NULL)
    {
        // Save the next node before freeing the current first node.
        Nodo *new_first = queue->first->next;
        int dequeued_id = queue->first->id;
        free(queue->first);
        queue->first = new_first;
        return dequeued_id;
    }

    return -1;
}

/**
 * Destroys the queue, freeing every remaining node and applying the
 * destroy function to each dequeued id.
 */
void destroy_queue(Queue *queue ,DestroyFunction destroy )
{
    while(queue->first != NULL)
      destroy(dequeue(queue));
    free(queue);
}