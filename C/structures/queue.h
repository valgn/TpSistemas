#ifndef __QUEUE__H__
#define __QUEUE__H__
#include <pthread.h>


typedef void (*DestroyFunction)(void *data);

/**
 * Single node of the queue, holding an id and a pointer to the next node.
 */
typedef struct nodo
{
    void* id;
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
 * Adds an element with the given id to the end of the queue.
 */
void enqueue(Queue *queue, int id);

/**
 * Removes and returns the id at the front of the queue.
 */
void* dequeue(Queue *queue);

/**
 * Destroys the queue, freeing every remaining node and applying the
 * destroy function to each dequeued id.
 */
void destroy_queue(Queue *queue, DestroyFunction destr);

#endif