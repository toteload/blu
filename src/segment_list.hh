#pragma once

#include "toteload.hh"

// For this data structure I wanted to try the zig way, where the allocator for the data structure
// is not stored inside the struct.
// https://ziglang.org/documentation/master/std/#std.segmented_list.SegmentedList
//
// The items are stored in segments.
// To find the correct item given an index, we need to find the corresponding segment index
// and the index for the item in the segment, which I call a point index.
// 
// Segment index (si) = bitwidth(i + 1) - 1
// Point   index (pi) = i + 1 - (1 << si)
//
// Item indices per segment index 
//
// 0 |  0
// 1 |  1  2
// 2 |  3  4  5  6
// 3 |  7  8  9 10 11 12 13 14
// 4 | 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30
//
// Point indices per segment index
//
// 0 |  0
// 1 |  0  1
// 2 |  0  1  2  3
// 3 |  0  1  2  3  4  5  6  7
// 4 |  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
//
// TODO: optimize this to start segments from a larger size. zig even inlines the first segment into
// the data structure itself. That way heap allocation might not even be used.

template<typename T>
struct SegmentList {
  usize segment_count = 0;
  T **segments = nullptr;
  usize _len = 0;

  void init() {
    segment_count = 0;
    segments = nullptr;
    _len = 0;
  }

  void deinit(Allocator alloc);

  void append(Allocator alloc, T item);

  T *push(Allocator alloc);

  T *ptr_at_unchecked(usize i);

  T &at(usize i) {
    return *ptr_at_unchecked(i);
  }

  T &operator[](usize i) {
    return at(i);
  }

  usize len() { return _len; }
  usize capacity() {
    if (segment_count == 0) {
      return 0;
    }

    return (1 << segment_count) - 1;
  }

  // `new_capacity` must be a power of two.
  void ensure_capacity(Allocator alloc, usize new_capacity);

  static usize segment_count_at_capacity(usize cap) {
    return bitwidth(cap);
  }

  // Return the size of the segment at index `i` in the `segments` array.
  static usize segment_size_at_segment_index(usize i) {
    return 1 << i;
  }

  static usize segment_idx(usize i) {
    return bitwidth(i + 1) - 1;
  }

  static usize point_index(usize i, usize si) {
    return i + 1 - (1 << si);
  }
};

template<typename T>
void SegmentList<T>::deinit(Allocator alloc) {
  if (!segments) {
    return;
  }

  for (usize i = 0; i < segment_count; i++) {
    alloc.free<T>(segments[i], 1 << i);
  }

  alloc.free(segments, segment_count);
}

template<typename T>
T *SegmentList<T>::ptr_at_unchecked(usize i) {
  usize si = segment_idx(i);
  usize pi = point_index(i, si);
  return &segments[si][pi];
}

template<typename T>
T *SegmentList<T>::push(Allocator alloc) {
  ensure_capacity(alloc, round_up_to_nearest_power_of_two(_len + 1));
  T *p = ptr_at_unchecked(_len);
  _len += 1;
  return p;
}

template<typename T>
void SegmentList<T>::append(Allocator alloc, T item) {
  *push(alloc) = item;
}

template<typename T>
void SegmentList<T>::ensure_capacity(Allocator alloc, usize new_capacity) {
  Debug_assert(new_capacity != 0 && is_zero_or_power_of_two(new_capacity));

  usize current_capacity = capacity();
  if (new_capacity <= current_capacity) {
    return;
  }

  usize new_segment_count = segment_count_at_capacity(new_capacity);

  T **new_segments = alloc.realloc(segments, segment_count, new_segment_count);

  for (usize i = segment_count; i < new_segment_count; i++) {
    new_segments[i] = alloc.alloc<T>(segment_size_at_segment_index(i));
  }

  segment_count = new_segment_count;
  segments = new_segments;
}
