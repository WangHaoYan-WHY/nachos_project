#include "random.h"
#include <iostream>

// LN constructor
template <class T>
LNode<T>::LNode(T k) {
  val = k;
  next = nullptr;
  last = nullptr;
}

// constructor
template <class T>
random_Structure<T>::random_Structure(int l, T t1, T t2) {
  head = new LNode<T>(t1); tail = new LNode<T>(t2);
  head->next = tail; tail->last = head;
  len = l;
  cur = 0;
}

// insert value into the random structure
template <class T>
void random_Structure<T>::set(T value) {
  if (ma.find(value) != ma.end()) {
    return;
  }
  if (len <= 0) {
    return;
  }
  if (cur == len) {
    remove();
  }
  // insert the new node into the tail of the list
  LNode<T> *node = new LNode<T>(value);
  ma[value] = node;
  ma[value]->last = tail->last;
  ma[value]->next = tail;
  tail->last->next = ma[value];
  tail->last = ma[value];
  ++cur; // increase the nodes' number
}

// remove the element from the random structure
template <class T>
T random_Structure<T>::remove()
{
  int num = rand() % (cur);
  LNode<T>* node = head;
  for (int i = 0; i <= num; i++) {
    node = node->next;
  }
  T store = node->val; // get val
  node->next->last = node->last;
  node->last->next = node->next;
  ma.erase(store); // erase from map
  --cur;
  return store;
}
