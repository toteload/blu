#pragma once

#include "toteload.hh"
#include <limits>

template<typename T, typename Tag>
struct OptionalIndex;

template<typename T, typename Tag>
struct Index {
  T idx;

  static Index from_optional_index(OptionalIndex<T, Tag> idx);

  OptionalIndex<T, Tag> as_optional_index();
  T get() { return idx; }

  bool operator==(const Index &other) {
    return idx == other.idx;
  }

  bool operator!=(const Index &other) {
    return idx != other.idx;
  }
};

template<typename T, typename Tag>
struct OptionalIndex {
  static constexpr T none_idx = std::numeric_limits<T>::max();
  static constexpr OptionalIndex none() { return { none_idx }; }

  T idx;

  static OptionalIndex from_index(Index<T, Tag> idx);

  Index<T, Tag> as_index();
  b32 is_none() { return idx == none_idx; }
  T get() {
    Assert(!is_none());
    return idx;
  }
};

template<typename T, typename Tag>
Index<T,Tag> Index<T, Tag>::from_optional_index(OptionalIndex<T, Tag> idx) {
  Assert(!idx.is_none());
  return { idx.idx };
}

template<typename T, typename Tag>
OptionalIndex<T,Tag> Index<T,Tag>::as_optional_index() {
  return OptionalIndex<T,Tag>::from_index(*this);
}

template<typename T, typename Tag>
OptionalIndex<T,Tag> OptionalIndex<T,Tag>::from_index(Index<T,Tag> idx) {
  return {idx.idx};
}

template<typename T, typename Tag>
Index<T,Tag> OptionalIndex<T,Tag>::as_index() {
  return {idx};
}

