#pragma once
#ifndef RANDOM_H
#define RANDOM_H
#include <map>

// class is the node stored in random data structure
template <class T>
class LNode {
public:
  LNode * next, *last; // next node and last node
  T val; // value
  LNode(T k); // constructor
};

// random data structure
template <class T>
class random_Structure {
public:
  random_Structure(int l, T t1, T t2); // constructor

  void set(T value); // insert value

  T remove(); // remove one
private:
  std::map<T, LNode<T>*> ma; // map
  LNode<T> *head, *tail; // head node and tail node
  int len;
  int cur;
};

#include "random.cc"
#endif