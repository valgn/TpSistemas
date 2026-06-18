#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

typedef void *(*CopyFunction)(void *data);
/** Returns a physical copy of the data */
typedef int (*CompareFunction)(void *data1, void *data2);
/** Returns a negative integer if data1 < data2, 0 if they are equal, and a
 * positive integer if data1 > data2 */
typedef void (*DestroyFunction)(void *data);
/** Frees the memory allocated for the data */
typedef unsigned (*HashFunction)(void *data);
/** Returns an unsigned integer for the data */

typedef struct _HashTable *HashTable;

/**
 * Creates a new empty hash table with the given capacity.
 */
HashTable  hashTable_create(unsigned capacity, CopyFunction copy,
                          CompareFunction comp, DestroyFunction destr,
                          HashFunction hash);

/**
 * Returns the number of elements in the table.
 */
int hashTable_nelems(HashTable  table);

/**
 * Returns the capacity of the table.
 */
int hashTable_capacity(HashTable  table);

/**
 * Destroys the table.
 */
void hashTable_destroy(HashTable  table);

/**
 * Inserts a data item into the table, or replaces it if it was already present.
 */
void hashTable_insert(HashTable  table, void *data);

/**
 * Returns the data from the table that matches the given data, or NULL if the
 * searched data is not found in the table.
 */
void *hashTable_search(HashTable  table, void *data);

/**
 * Deletes the data from the table that matches the given data.
 */
void hashTable_delete(HashTable  table, void *data);


#endif /* __HASHTABLE_H__*/