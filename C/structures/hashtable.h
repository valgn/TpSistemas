#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

#include "glist.h"

typedef int (*PredicateFunction)(void *data1,void *data2);
typedef int (*CompareFunction)(void *data1, void *data2);
typedef void (*DestroyFunction)(void *data);
typedef void* (*CopyFunction)(void *data);
typedef unsigned (*HashFunction)(void *data);

typedef struct {
  GList list;
} HashSlot;

typedef struct _HashTable {
  HashSlot *elems;
  unsigned numElems;
  unsigned capacity;
  CopyFunction copy;
  CompareFunction comp;
  DestroyFunction destr;
  HashFunction hash;
} *HashTable;

HashTable  hashTable_create(unsigned capacity, CopyFunction copy,
                          CompareFunction comp, DestroyFunction destr,
                          HashFunction hash);

int hashTable_nelems(HashTable  table);

int hashTable_capacity(HashTable  table);

void hashTable_destroy(HashTable  table);

void hashTable_insert(HashTable  table, void *data);

void *hashTable_search(HashTable  table, void *data);

void hashTable_delete(HashTable  table, void *data);

void hashTable_remove_if(HashTable table, PredicateFunction predi , void *data);

void hashTable_search_and_destroy_front(HashTable table, void *data);

#endif