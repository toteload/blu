#pragma once

#include "toteload.hh"
#include <limits>

template<typename T, typename Tag>
struct Index {
  T idx;

  T inner() const { return idx; }

  bool operator==(const Index &other) {
    return idx == other.idx;
  }

  bool operator!=(const Index &other) {
    return idx != other.idx;
  }

  bool is_some() const { return idx != 0; }
  bool is_none() const { return idx == 0; }

  static Index none() {
    return { .idx = 0, };
  }
};

