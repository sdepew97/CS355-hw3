/* llist.c

   Implementation of an singly linked list sequence
*/

#include <stdlib.h>

#include "llist.h"
#include "llist_node.h"

typedef struct llist_record {
    llist_node *head;
    int size;  // number of nodes in the llist
} llist_record;

// return an empty llist. Make sure to call llist_free when you're done
// with it.
llist llist_new() {
  llist l = malloc(sizeof(llist_record));
  l->head = NULL;
  l->size = 0;
  return l;
}

// returns the number of elements in the llist. Runs in constant time.
int llist_size(llist l) {
  return l->size;
}

// appends a new element to the beginning of the llist. Runs in constant time.
void llist_push(llist l, node_information elt) {
  l->head = new_node(elt, l->head);
  l->size++;
}

// removes and returns the first element in the llist. Runs in constant time.
// precondition: there is at least one element in the llist.
node_information llist_pop(llist l) {
  llist_node *old_head = l->head;
  node_information val = old_head->data;
  l->head = old_head->next;
  l->size--;

  free(old_head);
  return val;
}

// returns the first element in the llist, without changing the llist.
// Runs in constant time.
// precondition: there is at least one element in the llist.
node_information llist_peek(llist l) {
  return l->head->data;
}

// adds a new element into the llist, after n existing elements.
// Takes O(n) steps.
// precondition: the list has at least n elements
void llist_insert(llist l, int n, node_information elt) {
  if (n == 0) {
    llist_push(l, elt);
  } else {
    llist_node *one_before = nth_node(l->head, n - 1);
    insert_after(one_before, elt);
    l->size++;
  }
}

// retrieves the nth element of the llist. Takes O(n) steps.
// precondition: the list has at least (n+1) elements
node_information llist_get(llist l, int n) {
  return nth_node(l->head, n)->data;
}

// sets the nth element of the llist to a new value. Takes O(n) steps.
// precondition: the list has at least (n+1) elements
// postcondition: returns the old value of the element that was set
node_information llist_set(llist l, int n, node_information new_elt) {
  llist_node *node = nth_node(l->head, n);
  node_information old_elt = node->data;
  node->data = new_elt;
  return old_elt;
}

// removes the nth element of the llist. Takes O(n) steps.
// precondition: the list has at least (n+1) elements
// postcondition: returns the removed element
node_information llist_remove(llist l, int n) {
  if (n == 0) {
    return llist_pop(l);
  } else {
    llist_node *one_before = nth_node(l->head, n - 1);
    node_information elt = one_before->next->data;
    delete_after(one_before);
    l->size--;
    return elt;
  }
}

// frees an llist. Takes O(size(l)) steps.
void llist_free(llist l)
{
  free_llist(l->head);
  free(l);
}
