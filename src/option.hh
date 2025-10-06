#pragma once

#include "toteload.hh"

template<typename T>
struct Option {
  bool has_val;
  T val;

  Option(): has_val(false) {}
  Option(T x): has_val(true), val(x) {}

  bool is_some() const { return has_val; }
  T get_unchecked() { return val; }
  T get() {
    Assert(has_val);
    return val;
  }

  void set(T x) { has_val = true; val = x; }
};
