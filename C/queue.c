#include "queue.h"
#include <stdio.h>
#include <stdlib.h>


Queue *crear_cola()
{
    Queue *cola = malloc(sizeof(Queue));
    cola->primero = NULL;
    cola->ultimo = NULL;
    return cola;
}

void encolar(Queue *cola, int id)
{
    Nodo *nodo = malloc(sizeof(Nodo));
    nodo->id = id;
    nodo->next = NULL;
    if(cola->primero == NULL)
    {
        cola->primero = nodo;
        cola->ultimo= nodo;
    }
    else 
    {
        cola->ultimo->next = nodo;
        cola->ultimo = nodo;
    }
}

void* desencolar(Queue *cola)
{
    if (cola->primero != NULL)
    {
        Nodo *nuevo_primero = cola->primero->next;
        int id_desencolado = cola->primero->id;
        free(cola->primero);
        cola->primero = nuevo_primero;
        return id_desencolado;
    }

    return -1;
}

void destruir_cola(Queue *cola ,FuncionDestructora destruir )
{
    while(cola->primero != NULL)
      destruir(desencolar(cola));
    free(cola);
}