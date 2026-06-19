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

/**
 * Returns an empty list.
 */
GList glist_create();

/**
 * Destroys the list.
 */
void glist_destroy(GList list, DestroyFunction);

/**
 * Determines whether the list is empty.
 */
int glist_isEmpty(GList list);

/**
 * Adds an element at the front of the list.
 */
GList glist_addFront(GList list, void *data, CopyFunction);

/**
 * Searches for an element in the list.
 */
void* glist_search(GList list, void* data, CompareFunction);

/**
 * Traverses the list, applying the given function.
 */
void glist_traverse(GList list, VisitFunction);

GList glist_delete(GList list, void* data, DestroyFunction destr, CompareFunction comp);

int glist_count(GList list);

GList glist_addBack(GList list, void *data, CopyFunction copy);
#endif /* __GLIST_H__ */