#pragma once
#ifndef LRU_H
#define LRU_H
#include <map>

// class is the node stored in LRU data structure
template <class T>
class LN {
public:
  LN * next, *last; // next node and last node
  T val; // value
  T key; // key
  LN(T k, T v); // constructor
};

// LRU data structure
template <class T>
class LRU_Structure {
public:
  LRU_Structure(int capacity, T t1, T t2); // constructor

  T get(T key); // get the corresponding value whose key is key

  void set(T key, T value); // set the node into LRU with its key and value

  T removeFront(); // remove front node in LRU, front node is least recent used
private:
  std::map<T, LN<T>*> ma; // map
  LN<T> *head, *tail; // head node and tail node
  int cur, cp; // cur is the current nodes' number, cp is the capacity of the LRU
};

#include "LRU.cc"
#endif