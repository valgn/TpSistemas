#include "glist.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * Returns an empty list.
 */
GList glist_create() { 
  return NULL; 
}

/**
 * Destroys the list.
 */
void glist_destroy(GList list, DestroyFunction destroy) {
  GNode *nodeToDelete;
  while (list != NULL) {
    nodeToDelete = list;
    list = list->next;
    destroy(nodeToDelete->data); 
    free(nodeToDelete);
  }
}

/**
 * Determines whether the list is empty.
 */
int glist_isEmpty(GList list) { 
  return (list == NULL); 
}

/**
 * Adds an element at the front of the list.
 */
GList glist_addFront(GList list, void *data, CopyFunction copy) {
  GNode *newNode = malloc(sizeof(GNode));
  assert(newNode != NULL);
  newNode->next = list;
  newNode->data = copy(data);
  return newNode;
}

GList glist_addBack(GList list, void* data, CopyFunction copy){
    if(list==NULL){
        GNode *newNode = malloc(sizeof(GNode));
        newNode->data = copy(data);
        newNode->next = NULL;
        return newNode;
    }
    list->next = glist_addBack(list->next, data, copy);
    return list;
}

/**
 * Searches for an element in the list.
 */
void* glist_search(GList list, void* data, CompareFunction comp){
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
 * Traverses the list, applying the given function.
 */
void glist_traverse(GList list, VisitFunction visit) {
  for (GNode *node = list; node != NULL; node = node->next)
    visit(node->data);
}

GList glist_delete(GList list, void* data, DestroyFunction destr, CompareFunction comp){
  GNode* prev = NULL;
  GNode* curr = list;
  int found = 1;
  while(curr != NULL && found){
    if(comp(curr->data, data) == 0) found = 0;
    else {
      prev = curr;
      curr = curr->next;
    }
  }
  if(!found){
    GNode* temp;
    if(prev == NULL){
      temp = curr;
      list = curr->next;
      destr(temp->data);
      free(temp);
    }
    else{
      temp = curr;
      prev->next = curr->next;
      destr(temp->data);
      free(temp);
    }
  }
  return list;
}

int glist_count(GList list){
  int count = 0;
  if(list == NULL) return 0;
  while(list != NULL){
    count++;
    list = list->next;
  }
  return count;
}