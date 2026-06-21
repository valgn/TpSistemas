#ifndef __QUEUE__H__
#define __QUEUE__H__
#include <pthread.h>


typedef void (*DestroyFunction)(void *data);
typedef void *(*CopyFunction)(void *data);

/**
 * Single node of the queue, holding a data and a pointer to the next node.
 */
typedef struct nodo
{
    void* data;
    struct nodo *next;
} Nodo;

/**
structure representing the queue, with pointers to the first and last nodes
 */
typedef struct queue
{   
    Nodo *first;
    Nodo *last;
} Queue;


/**
 * Creates a new empty queue.
 */
Queue *create_queue();

/**
 * Adds an element to the end of the queue.
 */
void enqueue(Queue *queue, void* data, CopyFunction copy )

/**
 * Removes and returns the data at the front of the queue.
 */
void* dequeue(Queue *queue, CopyFunction copy, DestroyFunction destroy )

/**
 * Destroys the queue, freeing every remaining node and applying the
 * destroy function to each dequeued data.
 */
void destroy_queue(Queue *queue, DestroyFunction destr);

#endif