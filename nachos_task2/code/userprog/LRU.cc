#include "LRU.h"
#include <iostream>

// LN constructor
template <class T>
LN<T>::LN(T k, T v) {
  key = k;
  val = v;
  next = nullptr;
  last = nullptr;
}

// LRU data structure constructor
template <class T>
LRU_Structure<T>::LRU_Structure(int capacity, T t1, T t2) {
  cur = 0; // current nodes' number is 0
  cp = capacity; // capacity of LRU is capacity
  head = new LN<T>(t1, t1); // head node
  tail = new LN<T>(t2, t2); // tail node
  head->next = tail; // set head's next node as tail
  tail->last = head; // set tail's last node as head
}

// remove front element in LRU, front element is the least recent used
template <class T>
T LRU_Structure<T>::removeFront() {
  T store = head->next->key; // get the key of the front node, here head and tail are virtual nodes
  head->next->next->last = head;
  head->next = head->next->next;
  ma.erase(store); // erase the front node from map
  --cur;
  return store;
}

// get the corresponding value of the key from LRU
template <class T>
T LRU_Structure<T>::get(T key) {
  if (ma.find(key) == ma.end()) {
    return NULL;
  }
  ma[key]->last->next = ma[key]->next;
  ma[key]->next->last = ma[key]->last;
  ma[key]->last = tail->last;
  tail->last->next = ma[key];
  ma[key]->next = tail;
  tail->last = ma[key];
  return ma[key]->val;
}

// set the node whose key is key, value is value into LRU
template <class T>
void LRU_Structure<T>::set(T key, T value) {
  if (get(key) != NULL) {
    ma[key]->val = value;
    return;
  }
  if (cp <= 0) {
    return;
  }
  if (cur == cp) { // the LRU is full
    removeFront(); // remove front node
  }
  // insert the new node into the tail of the list
  LN<T> *node = new LN<T>(key, value);
  ma[key] = node;
  ma[key]->last = tail->last;
  ma[key]->next = tail;
  tail->last->next = ma[key];
  tail->last = ma[key];
  ++cur; // increase the nodes' number in LRU
}

