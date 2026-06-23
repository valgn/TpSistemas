#include "queue.h"
#include <stdlib.h>



/**
    * Creates a new empty queue and returns a pointer to it.
    * parameters: none
    * returns: pointer to the new queue
 */
Queue *create_queue()
{
    Queue *queue = malloc(sizeof(Queue));
    queue->first = NULL;
    queue->last = NULL;
    return queue;
}

/**
    * Adds a new element to the end of the queue.
    * parameter: queue - pointer to the queue, data - pointer to the data to be added, copy - function to copy the data
    * returns: none
 */
void enqueue(Queue *queue, void* data, CopyFunction copy )
{
    Nodo *nodo = malloc(sizeof(Nodo));
    nodo->data = copy(data);
    nodo->next = NULL;
    if(queue->first == NULL)
    {
        queue->first = nodo;
        queue->last = nodo;
    }
    else
    {
        queue->last->next = nodo;
        queue->last = nodo;
    }
}



/**
    * Removes the first element from the queue and returns its data.
    * parameter: queue - pointer to the queue, copy - function to copy the data, destroy - function to destroy the data
    * returns: pointer to the data of the removed element
 */
void* dequeue(Queue *queue, CopyFunction copy, DestroyFunction destroy )
{
    if (queue == NULL || queue->first == NULL)
        return NULL;

    Nodo *old_first = queue->first;
    void *data = old_first->data;
    queue->first = old_first->next;
    if (queue->first == NULL) {
        queue->last = NULL;
    }

    if (copy != NULL)
        data = copy(data);

    if (destroy != NULL)
        destroy(old_first->data);

    free(old_first);
    return data;
}
/**
    * Checks if the queue is empty.
    * parameter: queue - pointer to the queue
    * returns: 1 if the queue is empty, 0 otherwise
 */

int empty_queue(Queue *queue)
{
    return queue == NULL || queue->first == NULL;
}

/**
    * Removes elements from the queue that satisfy a given predicate function.
    * parameter: queue - pointer to the queue, pred - predicate function, ctx - context for the predicate, destroy - function to destroy the data
    * returns: 1 if any elements were removed, 0 otherwise
 */

int queue_remove_if(Queue *queue, PredicateFunction pred, void *ctx, DestroyFunction destroy)
{
    if (queue == NULL || pred == NULL)
        return 0;

    int removed = 0;
    Nodo *curr = queue->first;
    Nodo *prev = NULL;

    while (curr)
    {
        if (pred(curr->data, ctx))
        {
            Nodo *to_delete = curr;
            if (prev == NULL)
                queue->first = curr->next;
            else
                prev->next = curr->next;

            if (curr == queue->last)
                queue->last = prev;

            curr = curr->next;
            if (destroy) destroy(to_delete->data);
            free(to_delete);
            removed = 1;
        }
        else
        {
            prev = curr;
            curr = curr->next;
        }
    }

    return removed;
}


/**
    * Destroys the queue and frees all associated memory.
    * parameter: queue - pointer to the queue, copy - function to copy the data, destroy - function to destroy the data
    * returns: none
 */
void destroy_queue(Queue *queue, CopyFunction copy, DestroyFunction destroy)
{
    if (queue == NULL)
        return;

    while (queue->first != NULL) {
        dequeue(queue, copy, destroy);
    }
    free(queue);
}