#pragma once

#include "vector.hh"

template<typename T>
struct Queue {
  Vector<T> backing;

  void init(Allocator alloc) {
    backing.init(alloc);
  }

  void deinit() {
    backing.deinit();
  }

  void push_back(T x) {
    backing.push(x);
  }

  T pop_front() {
    return backing.shift_left();
  }
};
