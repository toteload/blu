#ifndef SEGMENTLIST_NAME
#error "SEGMENTLIST_NAME must be defined"
#endif

#ifndef SEGMENTLIST_MIN_SIZE_LOG2
#error "SEGMENTLIST_MIN_SIZE_LOG2 must be defined"
#endif

#ifndef SEGMENTLIST_SEGMENT_COUNT
#error "SEGMENTLIST_SEGMENT_COUNT must be defined"
#endif

#include "toteload.h"

// This is an alternative way of defining a SegmentList that takes less
// templating than the current approach.

#ifdef SEGMENTLIST_OUTPUT_TYPES

#ifndef SEGMENTLIST_HEADER_DEFINED
#define SEGMENTLIST_HEADER_DEFINED

typedef struct {
  u32 len;
  u32 active_segment_count;
  u32 min_size_log2;
  u32 max_segment_count;
  void *segments[];
} SegmentList__Header;

#endif // SEGMENTLIST_HEADER_DEFINED

typedef struct {
  SegmentListHeader header;
  void *segments[SEGMENTLIST_SEGMENT_COUNT];
} SEGMENTLIST_NAME;

#undef SEGMENTLIST_OUTPUT_TYPES
#endif // SEGMENTLIST_OUTPUT_TYPES

void Cat(SEGMENTLIST_NAME, _init)(SEGMENTLIST_NAME *list) {
  *list = (SEGMENTLIST_NAME){
    .header = {
      .len = 0,
      .active_segment_count = 0,
      .min_size_log2 = SEGMENTLIST_MIN_SIZE_LOG2,
      .max_segment_count = SEGMENTLIST_SEGMENT_COUNT,
    },
    .segments = { 0 },
  };
}

#ifdef SEGMENTLIST_OUTPUT_DEFINITIONS

#define segment_list_ptr_at_unchecked(type, list, idx) \
  segment_list__ptr_at_unchecked(list, sizeof(type), idx)

#define segment_list_push(type, list, arena) \
  Cast(type*,segment_list__push(list, arena, sizeof(type)))

internal usize segment_count_at_capacity(usize min_size_log2, usize cap) {
  return bitwidth(cap >> min_size_log2) + 1;
}

internal usize segment_size(usize min_size_log2, usize si) {
  return (usize)1 << (min_size_log2 + si);
}

internal usize segment_idx(usize min_size_log2, usize i) {
  return bitwidth((i >> min_size_log2) + 1) - 1;
}

internal usize item_idx(usize min_size_log2, usize i, usize si) {
  return i + ((usize)1 << min_size_log2) - ((usize)1 << (min_size_log2 + si));
}

usize segment_list__cap(SegmentList__Header *list) {
  return (((usize)1 << list->active_segment_count) - 1) << list->min_size_log2;
}

void segment_list__ensure_capacity(SegmentList__Header *list, Arena *arena, usize min_capacity, usize item_size) {
  usize cap = segment_list__cap(list);
  if (cap >= min_capacity) {
    return;
  }

  usize required_segment_count = segment_count_at_capacity(list->min_size_log2, min_capacity);
  if (required_segment_count <= list->active_segment_count) {
    return;
  }

  for (usize i = list->active_segment_count; i < required_segment_count; i++) {
    usize size = segment_size(list->min_size_log2, i);
    list->segments[i] = arena_push_array(arena, item_size, size);
  }

  list->active_segment_count = required_segment_count;
}

void *segment_list__ptr_at_unchecked(SegmentList__Header *list, usize item_size, usize idx) {
  usize si = segment_idx(list->min_size_log2, idx);
  usize ii = item_idx(list->min_size_log2, idx, si);

  return ptr_offset(list->segments[si], ii * item_size);
}

void *segment_list__push(SegmentList__Header *list, Arena *arena, usize item_size) {
  segment_list__ensure_capacity(list, arena, item_size);
  void *p = segment_list__ptr_at_unchecked(list, item_size, list->len++);
  list->len += 1;
  return p;
}

#undef SEGMENTLIST_OUTPUT_DEFINITIONS
#endif // SEGMENTLIST_OUTPUT_DEFINITIONS

#undef SEGMENTLIST_NAME
#undef SEGMENTLIST_MIN_SIZE_LOG2
#undef SEGMENTLIST_SEGMENT_COUNT
