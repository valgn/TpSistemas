#include "hashtable.h"
#include <assert.h>
#include <stdlib.h>
#include "glist.h"

/**
 * Slots in which we will store the hash table data.
 */
typedef struct {
  GList list;
} HashSlot;

/**
 * Main structure that represents the hash table.
 */
struct _HashTable {
  HashSlot *elems;
  unsigned numElems;
  unsigned capacity;
  CopyFunction copy;
  CompareFunction comp;
  DestroyFunction destr;
  HashFunction hash;
};



/**
 * Creates a new empty hash table with the given capacity.
 */
HashTable hashTable_create(unsigned capacity, CopyFunction copy,
                          CompareFunction comp, DestroyFunction destr,
                          HashFunction hash) {

  // Request memory for the main structure and the slots.
  HashTable table = malloc(sizeof(struct _HashTable));
  assert(table != NULL);
  table->elems = malloc(sizeof(HashSlot) * capacity);
  assert(table->elems != NULL);
  table->numElems = 0;
  table->capacity = capacity;
  table->copy = copy;
  table->comp = comp;
  table->destr = destr;
  table->hash = hash;

  for (unsigned idx = 0; idx < capacidad; ++idx) {
    tabla->elems[idx].list = glist_create();
  }

  return table;
}

/**
 * Returns the number of elements in the table.
 */
int hashTable_nelems(HashTable table) { return table->numElems; }

/**
 * Returns the capacity of the table.
 */
int hashTable_capacity(HashTable table) { return table->capacity; }

/**
 * Destroys the table.
 */
void hashTable_destroy(HashTable table) {

  // Destroy each of the slots' data.
  for (unsigned idx = 0; idx < table->capacity; ++idx)
      glist_destroy(table->elems[idx].list, table->destr);
  // Free the slot array and the table.
  free(table);
  return;
}

/**
 * Inserts a data item into the table, or replaces it if it was already present.
 * IMPORTANT: This implementation does not handle collisions.
 */
void hashTable_insert(HashTable table, void *data) {

  // Compute the position of the given data, according to the hash function.
  unsigned idx = table->hash(data) % table->capacity;

    table->elems[idx].list = glist_addFront(table->elems[idx].list, data, table->copy);
    table->numElems++;

}

/**
 * Returns the data from the table that matches the given data, or NULL if the
 * searched data is not found in the table.
 */
void *hashTable_search(HashTable table, void *data) {

  // Compute the position of the given data, according to the hash function.
  unsigned idx = table->hash(data) % table->capacity;
    return glist_search(table->elems[idx].list, data, table->comp);
}

/**
 * Deletes the data from the table that matches the given data.
 */
void hashTable_delete(HashTable table, void *data) {

  // Compute the position of the given data, according to the hash function.
  unsigned idx = table->hash(data) % table->capacity;

    table->elems[idx].list = glist_delete(table->elems[idx].list, data, table->destr, table->comp);
    table->numElems--;

}