#ifndef __QUEUE__H__
#define __QUEUE__H__
#include <pthread.h>

typedef void (*FuncionDestructora)(void *dato);

typedef struct nodo
{
    void* id;
    struct nodo *next;
} Nodo;

typedef struct queue
{   
    Nodo *primero;
    Nodo *ultimo;
} Queue;

Queue *crear_cola();
void encolar(Queue *cola, int id);
void* desencolar(Queue *cola);
void destruir_cola(Queue *cola, FuncionDestructora destruir);

#endif