/* llist.h

   Declarations for an abstract sequence type backed by a singly linked list.
*/

#ifndef LLIST_H_
#define LLIST_H_

#include "llist_node.h"

// An abstract type for sequences
typedef struct llist_record* llist;

// return an empty llist. Make sure to call llist_free when you're done
// with it.
llist llist_new();

// returns the number of elements in the llist. Runs in constant time.
int llist_size(llist l);

// appends a new element to the beginning of the llist. Runs in constant time.
void llist_push(llist l, node_information elt);

// removes and returns the first element in the llist. Runs in constant time.
// precondition: there is at least one element in the llist.
node_information llist_pop(llist l);

// returns the first element in the llist, without changing the llist.
// Runs in constant time.
// precondition: there is at least one element in the llist.
node_information llist_peek(llist l);

// adds a new element into the llist, after n existing elements.
// Takes O(n) steps.
// precondition: the list has at least n elements
void llist_insert(llist l, int n, node_information elt);

// retrieves the nth element of the llist. Takes O(n) steps.
// precondition: the list has at least (n+1) elements
node_information llist_get(llist l, int n);

// sets the nth element of the llist to a new value. Takes O(n) steps.
// precondition: the list has at least (n+1) elements
// postcondition: returns the old value of the element that was set
node_information llist_set(llist l, int n, node_information new_elt);

// removes the nth element of the llist. Takes O(n) steps.
// precondition: the list has at least (n+1) elements
// postcondition: returns the removed element
node_information llist_remove(llist l, int n);

// frees an llist. Takes O(size(l)) steps.
void llist_free(llist l);

#endif
