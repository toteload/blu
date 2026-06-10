#ifndef SEGMENTLIST_NAME
#error "SEGMENTLIST_NAME must be defined"
#endif

#ifndef SEGMENTLIST_TYPE
#error "SEGMENTLIST_TYPE must be defined"
#endif

#ifndef SEGMENTLIST_MIN_SIZE_LOG2
#error "SEGMENTLIST_MIN_SIZE_LOG2 must be defined"
#endif

#ifndef SEGMENTLIST_SEGMENT_COUNT
#error "SEGMENTLIST_SEGMENT_COUNT must be defined"
#endif

#ifndef SEGMENTLIST_FUNCTION_PREFIX
#define SEGMENTLIST_FUNCTION_PREFIX SEGMENTLIST_NAME
#endif

#ifndef SEGMENTLIST_LINKAGE
#define SEGMENTLIST_LINKAGE
#endif

#include "toteload.h"

#ifdef SEGMENTLIST_OUTPUT_TYPES

typedef struct {
  usize len;
  usize segment_count;
  SEGMENTLIST_TYPE *segments[SEGMENTLIST_SEGMENT_COUNT];
} SEGMENTLIST_NAME;

#undef SEGMENTLIST_OUTPUT_TYPES
#endif // SEGMENTLIST_OUTPUT_TYPES

#if defined(SEGMENTLIST_OUTPUT_DECLARATIONS) || defined(SEGMENTLIST_OUTPUT_DEFINITIONS)

SEGMENTLIST_LINKAGE usize             Cat(SEGMENTLIST_FUNCTION_PREFIX, _cap)(SEGMENTLIST_NAME *list);
SEGMENTLIST_LINKAGE SEGMENTLIST_TYPE *Cat(SEGMENTLIST_FUNCTION_PREFIX, _push)(SEGMENTLIST_NAME *list, Arena *arena);
SEGMENTLIST_LINKAGE void              Cat(SEGMENTLIST_FUNCTION_PREFIX, _append)(SEGMENTLIST_NAME *list, Arena *arena, SEGMENTLIST_TYPE item);
SEGMENTLIST_LINKAGE SEGMENTLIST_TYPE *Cat(SEGMENTLIST_FUNCTION_PREFIX, _ptr_at_unchecked)(SEGMENTLIST_NAME *list, usize i);

#undef SEGMENTLIST_OUTPUT_DECLARATIONS
#endif // SEGMENTLIST_OUTPUT_DECLARATIONS

#ifdef SEGMENTLIST_OUTPUT_DEFINITIONS

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

usize Cat(SEGMENTLIST_FUNCTION_PREFIX, _cap)(SEGMENTLIST_NAME *list) {
  return (((usize)1 << list->segment_count) - 1) << SEGMENTLIST_MIN_SIZE_LOG2;
}

internal void Cat(SEGMENTLIST_FUNCTION_PREFIX, __ensure_capacity)(SEGMENTLIST_NAME *list, Arena *arena, usize min_capacity) {
  usize cap = Cat(SEGMENTLIST_FUNCTION_PREFIX, _cap)(list);
  if (cap >= min_capacity) {
    return;
  }

  usize required_segment_count = segment_count_at_capacity(SEGMENTLIST_MIN_SIZE_LOG2, min_capacity);

  if (required_segment_count <= list->segment_count) {
    return;
  }

  for (usize i = list->segment_count; i < required_segment_count; i++) {
    usize size = segment_size(SEGMENTLIST_MIN_SIZE_LOG2, i);
    list->segments[i] = arena_push_array(arena, SEGMENTLIST_TYPE, size);
  }

  list->segment_count = required_segment_count;
}

SEGMENTLIST_LINKAGE
SEGMENTLIST_TYPE *Cat(SEGMENTLIST_FUNCTION_PREFIX, _push)(SEGMENTLIST_NAME *list, Arena *arena) {
  Cat(SEGMENTLIST_FUNCTION_PREFIX, __ensure_capacity)(list, arena, list->len + 1);
  SEGMENTLIST_TYPE *p = Cat(SEGMENTLIST_FUNCTION_PREFIX, _ptr_at_unchecked)(list, list->len);
  list->len += 1;
  return p;
}

SEGMENTLIST_LINKAGE
void Cat(SEGMENTLIST_FUNCTION_PREFIX, _append)(SEGMENTLIST_NAME *list, Arena *arena, SEGMENTLIST_TYPE item) {
  *Cat(SEGMENTLIST_FUNCTION_PREFIX, _push)(list, arena) = item;
}

SEGMENTLIST_LINKAGE
SEGMENTLIST_TYPE *Cat(SEGMENTLIST_FUNCTION_PREFIX, _ptr_at_unchecked)(SEGMENTLIST_NAME *list, usize idx) {
  usize si = segment_idx(SEGMENTLIST_MIN_SIZE_LOG2, idx);
  usize i  = item_idx(SEGMENTLIST_MIN_SIZE_LOG2, idx, si);

  return &list->segments[si][i];
}

#undef SEGMENTLIST_OUTPUT_DEFINITIONS
#endif // SEGMENTLIST_OUTPUT_DEFINITIONS

#undef SEGMENTLIST_NAME
#undef SEGMENTLIST_TYPE
#undef SEGMENTLIST_FUNCTION_PREFIX
#undef SEGMENTLIST_MIN_SIZE_LOG2
#undef SEGMENTLIST_SEGMENT_COUNT
#undef SEGMENTLIST_LINKAGE
