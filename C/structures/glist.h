#ifndef __GLIST_H__
#define __GLIST_H__

typedef void (*DestroyFunction)(void *data);
typedef void *(*CopyFunction)(void *data);
typedef void (*VisitFunction)(void *data);
typedef int (*CompareFunction)(void *data1, void *data2);
typedef int (*Predicate) (void *data);

typedef struct _GNode {
  void *data;
  struct _GNode *next;
} GNode;

typedef struct _GNode *GList;

/* -------------------------------------------------------------------
    * Creates an empty list.
    * parameter: none
    * returns: an empty list
*/
GList glist_create();
/**------------------------------------------------------------------
  * Destroys the list, applying the given destroy function to each element.
  * parameter: list - the list to destroy
  * parameter: destroy - a function to destroy each element
  * returns: none
 */
void glist_destroy(GList list, DestroyFunction);
/**------------------------------------------------------------------
  * Checks if the list is empty.
  * parameter: list - the list to check
  * returns: 1 if the list is empty, 0 otherwise
 */
int glist_isEmpty(GList list);
/**------------------------------------------------------------------
  * Adds an element to the front of the list.
  * parameter: list - the list to add to
  * parameter: data - the data to add
  * parameter: copy - a function to copy the data
  * returns: the new list with the element added
 */
GList glist_addFront(GList list, void *data, CopyFunction);
/**------------------------------------------------------------------
  * Searches for an element in the list using the given comparison function.
  * parameter: list - the list to search
  * parameter: data - the data to search for
  * parameter: comp - a function to compare elements
  * returns: the data if found, NULL otherwise
 */
void* glist_search(GList list, void* data, CompareFunction);
/**------------------------------------------------------------------
  * Traverses the list, applying the given visit function to each element.
  * parameter: list - the list to traverse
  * parameter: visit - a function to apply to each element
  * returns: none
 */
void glist_traverse(GList list, VisitFunction);
/**------------------------------------------------------------------
  * Deletes an element from the list using the given comparison function and destroy function.
  * parameter: list - the list to delete from
  * parameter: data - the data to delete
  * parameter: destr - a function to destroy the data
  * parameter: comp - a function to compare elements
  * returns: the new list with the element deleted
 */
GList glist_delete(GList list, void* data, DestroyFunction destr, CompareFunction comp);
/**------------------------------------------------------------------
  * Counts the number of elements in the list.
  * parameter: list - the list to count
  * returns: the number of elements in the list
 */
int glist_count(GList list);

GList glist_addBack(GList list, void *data, CopyFunction copy);
/**------------------------------------------------------------------
  * Destroys the front element of the list, applying the given destroy function to it.
  * parameter: list - the list to destroy the front element from
  * parameter: destroy - a function to destroy the front element
  * returns: the new list with the front element removed
 */
GList glist_destroyFront(GList list, DestroyFunction destroy);

#endif