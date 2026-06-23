#include "hashtable.h"
#include <assert.h>
#include <stdlib.h>

/*
  * Creates a new empty hash table with the given capacity.
 */
HashTable hashTable_create(unsigned capacity, CopyFunction copy,
                          CompareFunction comp, DestroyFunction destr,
                          HashFunction hash) 
{
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

  for (unsigned idx = 0; idx < capacity; ++idx) {
    table->elems[idx].list = glist_create();
  }

  return table;
}

/*
 * Returns the number of elements in the table.
 */
int hashTable_nelems(HashTable table) 
{ 
  return table->numElems; 
}

/*
 * Returns the capacity of the table.
 */
int hashTable_capacity(HashTable table) 
{ 
  return table->capacity; 
}

/*
 * Destroys the table.
 */
void hashTable_destroy(HashTable table) 
{
  for (unsigned idx = 0; idx < table->capacity; ++idx)
      glist_destroy(table->elems[idx].list, table->destr);
  free(table);
  return;
}

/*
 * Inserts a data item into the table, or replaces it if it was already present.
 */
void hashTable_insert(HashTable table, void *data) 
{
  unsigned idx = table->hash(data) % table->capacity;
  table->elems[idx].list = glist_addFront(table->elems[idx].list, data, table->copy);
  table->numElems++;
}

/*
 * Returns the data from the table that matches the given data, or NULL if the
 * searched data is not found in the table.
 */
void *hashTable_search(HashTable table, void *data) 
{
  unsigned idx = table->hash(data) % table->capacity;
  return glist_search(table->elems[idx].list, data, table->comp);
}

/*
  * Searchs the data desired and only removes the first element of the list in the index
*/
void hashTable_search_and_destroy_front(HashTable table, void *data) 
{
  unsigned idx = table->hash(data) % table->capacity;
  table->elems[idx].list = glist_destroyFront(table->elems[idx].list, table->destr);
}

/*
 * Deletes the data from the table that matches the given data.
 */
void hashTable_delete(HashTable table, void *data) 
{
  unsigned idx = table->hash(data) % table->capacity;
  table->elems[idx].list = glist_delete(table->elems[idx].list, data, table->destr, table->comp);
  table->numElems--;
}

/*
  * Removes data from a hash table if and only if the predicate is true
*/
void hashTable_remove_if(HashTable table, PredicateFunction pred, void *data) 
{
    if (table == NULL) return;

    for (unsigned idx = 0; idx < table->capacity; ++idx) {
        GNode *curr = table->elems[idx].list;
        GNode *prev = NULL;

        while (curr != NULL) {
            if (pred(curr->data, data)) {
                GNode *to_delete = curr;
                
                if (prev == NULL) {
                    table->elems[idx].list = curr->next;
                } else {
                    prev->next = curr->next;
                }
                
                curr = curr->next;

                table->destr(to_delete->data);
                free(to_delete);
                table->numElems--;
            } else {
                prev = curr;
                curr = curr->next;
            }
        }
    }
}