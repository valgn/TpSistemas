#ifndef ___AUXILIARY_FUNCTION_H___
#define ___AUXILIARY_FUNCTION_H___

#include "data_types.h"

/* -------------------------------------------------------------------
    * returns the same pointer passed as argument. Used for data structures that don't need to copy the data.
    * parametre void *data: pointer to the data to be copied
    * returns void *: pointer to the copied data
*/

void *identity_copy(void *data);

 /* -------------------------------------------------------------------
    * hash function for Job data structure. Used for data structures that need to hash the data.
    * parametre void *source: pointer to the data to be copied
    * returns void *: pointer to the copied data
*/
unsigned hash_job(void* source);
/* -------------------------------------------------------------------
    * comparison function for Job data structure. Used for data structures that need to compare the data.
    * parametre void *source1: pointer to the first data to be compared
    * parametre void *source2: pointer to the second data to be compared
    * returns int: -1 if source1 < source2, 0 if source1 == source2, 1 if source1 > source2
*/
int comp_job(void* source1, void* source2);
/* -------------------------------------------------------------------
    * hash function for NodeRequest data structure. Used for data structures that need to hash the data.
    * parametre void *source: pointer to the data to be copied
    * returns void *: pointer to the copied data 
*/
unsigned hash_nodeRequest(void* source);
/* -------------------------------------------------------------------
    * comparison function for NodeRequest data structure. Used for data structures that need to compare the data.
    * parametre void *source1: pointer to the first data to be compared
    * parametre void *source2: pointer to the second data to be compared
    * returns int: -1 if source1 < source2, 0 if source1 == source2, 1 if source1 > source2
*/
int comp_nodeRequest(void* source1, void* source2);
/* -------------------------------------------------------------------
    * destruction function for Job data structure. Used for data structures that need to destroy the data.
    * parametres void *source: pointer to the data to be destroyed
    * returns void
*/

void destroy_job(void* source);
/* -------------------------------------------------------------------
    * destruction function for NodeRequest data structure. Used for data structures that need to destroy the data.
    * parametres void *source: pointer to the data to be destroyed
    * returns void
*/
void destroy_nodeRequest(void* source);
/* -------------------------------------------------------------------
    * destruction function for Request data structure. Used for data structures that need to destroy the data.
    * parametres void *source: pointer to the data to be destroyed
    * returns void
*/
void destroy_request(void *source);
#endif