#include "glist.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * Devuelve una lista vacía.
 */
GList glist_crear() { 
  return NULL; 
}

/**
 * Destruccion de la lista.
 */
void glist_destruir(GList list, FuncionDestructora destroy) {
  GNode *nodeToDelete;
  while (list != NULL) {
    nodeToDelete = list;
    list = list->next;
    destroy(nodeToDelete->data); 
    free(nodeToDelete);
  }
}

/**
 * Determina si la lista es vacía.
 */
int glist_vacia(GList list) { 
  return (list == NULL); 
}

/**
 * Agrega un elemento al inicio de la lista.
 */
GList glist_agregar_inicio(GList list, void *data, FuncionCopia copy) {
  GNode *newNode = malloc(sizeof(GNode));
  assert(newNode != NULL);
  newNode->next = list;
  newNode->data = copy(data);
  return newNode;
}
GList glist_agregar_final(GList lista,void* dato,FuncionCopia copy){
    if(lista==NULL){
        GNode*nuevo= malloc(sizeof(GNode));
        nuevo->data=copy(dato);
        nuevo->next=NULL;
        return nuevo;
        
    }
    lista->next=  glist_agregar_final(lista->next,dato,copy);
    return lista;
}

/**
 * Busca un elemento en la lista.
 */
void* glist_buscar(GList list, void* data, FuncionComparar comp){
  int found = 0;
  GNode *node = list;
  while( found == 0 && node != NULL ){
    if (comp(node->data, data) == 0) found = 1;
    else node = node->next;
  }
  if(found){
    return node->data;
  }
  return NULL;
}

/**
 * Recorrido de la lista, utilizando la funcion pasada.
 */
void glist_recorrer(GList list, FuncionVisitante visit) {
  for (GNode *node = list; node != NULL; node = node->next)
    visit(node->data);
}

GList glist_eliminar(GList list,void* dato,FuncionDestructora destr,FuncionComparar comp){
  GNode* ant= NULL;
  GNode* aux= list;
  int bandera= 1;
  while(aux!= NULL&& bandera){
    if(comp(aux->data,dato)==0)bandera= 0;
    else {
      ant=aux;
      aux=aux->next;
    }
  }
  if(!bandera){
    GNode* temp;
    if(ant==NULL){
      temp= aux;
      list=aux->next;
      destr(temp->data);
      free(temp);
    }
    else{
      temp= aux;
      ant->next= aux->next;
      destr(temp->data);
      free(temp);
    }
  }
  return list;
}

int cant_elementos_glist(GList lista){
  int contador= 0;
  if(lista== NULL)return 0;
  while(lista!=NULL){
    contador++;
    lista= lista->next;
  }
  return contador;
}


