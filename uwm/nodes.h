#ifndef ARC_NODES_H
#define ARC_NODES_H

#include <stddef.h>

#ifndef False
#define False (0)
#endif
#ifndef True
#define True (-1)
#endif

typedef struct _Node Node;

struct _Node {
  struct _Node *next,*prev;
  void *data;
};

typedef struct _NodeList NodeList;

struct _NodeList {
  Node *first,*last;
};

/* returns pointer to node of data pointer or Null if ptr is not listed. */
Node* InNodeList(NodeList *list,void *ptr);

/* creates an empty list and returns a pointer to the corresponding structure */
NodeList *NodeListCreate();

/* appends a node to the given list, returns True on success, else false */
int NodeAppend(NodeList *list,void *ptr);

/* prepends a node to the given list, returns True on success, else false */
int NodeInsert(NodeList *list,void *ptr);

/* deletes an entry from a list */
Node* NodeDelete(NodeList *list,Node *node);

/* deletes a whole list. please note that the data entries of the nodes are not
   being freed - sets the list pointer to NULL after deleting the list */
void NodeListDelete(NodeList **list);

/* returns pointer to next node in list */
Node* NodeNext(NodeList *list,Node *node);

/* returns pointer to previous node in list */
Node* NodePrev(NodeList *list,Node *node);

/* returns number of nodes in list */
unsigned int NodeCount(NodeList *list);

/* moves specified node to end of list */
void Node2End(NodeList *list,Node *node);

/* moves specified node to beginning of list */
void Node2Start(NodeList *list,Node *node);

/* greater/equal/less-function returns >0 if a>b, 0 if a=b and <0 if a<b */
typedef int (*gelfunc)(void *a, void *b);

/* sorts the given NodeList */
void SortNodeList(NodeList *list, gelfunc order);

#endif
