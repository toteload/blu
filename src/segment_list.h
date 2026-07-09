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
SEGMENTLIST_LINKAGE SEGMENTLIST_TYPE  Cat(SEGMENTLIST_FUNCTION_PREFIX, _at_unchecked)(SEGMENTLIST_NAME *list, usize i);
SEGMENTLIST_LINKAGE void              Cat(SEGMENTLIST_FUNCTION_PREFIX, _copy_to_array)(SEGMENTLIST_NAME *list, SEGMENTLIST_TYPE *out);

#undef SEGMENTLIST_OUTPUT_DECLARATIONS
#endif // SEGMENTLIST_OUTPUT_DECLARATIONS

#ifdef SEGMENTLIST_OUTPUT_DEFINITIONS

#ifndef SEGMENTLIST_INTERNAL_FUNCTIONS_DEFINED
#define SEGMENTLIST_INTERNAL_FUNCTIONS_DEFINED

// Without the toggle of this preprocessor block, the functions defined here would be defined twice
// if you define two segment lists in the same translation unit leading to compile errors.

internal usize segment_count_at_size(usize min_size_log2, usize size) {
  return bitwidth(size >> min_size_log2) + 1;
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

#endif // SEGMENTLIST_INTERNAL_FUNCTIONS_DEFINED

usize Cat(SEGMENTLIST_FUNCTION_PREFIX, _cap)(SEGMENTLIST_NAME *list) {
  return (((usize)1 << list->segment_count) - 1) << SEGMENTLIST_MIN_SIZE_LOG2;
}

internal void Cat(SEGMENTLIST_FUNCTION_PREFIX, __ensure_capacity)(SEGMENTLIST_NAME *list, Arena *arena, usize min_capacity) {
  usize cap = Cat(SEGMENTLIST_FUNCTION_PREFIX, _cap)(list);
  if (cap >= min_capacity) {
    return;
  }

  usize required_segment_count = segment_count_at_size(SEGMENTLIST_MIN_SIZE_LOG2, min_capacity);

  if (required_segment_count <= list->segment_count) {
    return;
  }

  for (usize i = list->segment_count; i < required_segment_count; i++) {
    usize size = segment_size(SEGMENTLIST_MIN_SIZE_LOG2, i);
    list->segments[i] = arena_push_array(SEGMENTLIST_TYPE, arena, size);
  }

  list->segment_count = required_segment_count;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
SEGMENTLIST_LINKAGE
SEGMENTLIST_TYPE *Cat(SEGMENTLIST_FUNCTION_PREFIX, _push)(SEGMENTLIST_NAME *list, Arena *arena) {
  Cat(SEGMENTLIST_FUNCTION_PREFIX, __ensure_capacity)(list, arena, list->len + 1);
  SEGMENTLIST_TYPE *p = Cat(SEGMENTLIST_FUNCTION_PREFIX, _ptr_at_unchecked)(list, list->len);
  list->len += 1;
  return p;
}
#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
SEGMENTLIST_LINKAGE
void Cat(SEGMENTLIST_FUNCTION_PREFIX, _append)(SEGMENTLIST_NAME *list, Arena *arena, SEGMENTLIST_TYPE item) {
  *Cat(SEGMENTLIST_FUNCTION_PREFIX, _push)(list, arena) = item;
}
#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
SEGMENTLIST_LINKAGE
SEGMENTLIST_TYPE *Cat(SEGMENTLIST_FUNCTION_PREFIX, _ptr_at_unchecked)(SEGMENTLIST_NAME *list, usize idx) {
  usize si = segment_idx(SEGMENTLIST_MIN_SIZE_LOG2, idx);
  usize i  = item_idx(SEGMENTLIST_MIN_SIZE_LOG2, idx, si);

  return &list->segments[si][i];
}
#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
SEGMENTLIST_LINKAGE
SEGMENTLIST_TYPE Cat(SEGMENTLIST_FUNCTION_PREFIX, _at_unchecked)(SEGMENTLIST_NAME *list, usize idx) {
  return *Cat(SEGMENTLIST_FUNCTION_PREFIX, _ptr_at_unchecked)(list, idx);
}
#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
SEGMENTLIST_LINKAGE
void Cat(SEGMENTLIST_FUNCTION_PREFIX, _copy_to_array)(SEGMENTLIST_NAME *list, SEGMENTLIST_TYPE *out) {
  if (list->len == 0) {
    return;
  }

  u32 offset = 0;
  u32 segment_count = segment_count_at_size(SEGMENTLIST_MIN_SIZE_LOG2, list->len);
  for (u32 i = 0; i < segment_count - 1; i++) {
    u32 size = segment_size(SEGMENTLIST_MIN_SIZE_LOG2, i);
    memcpy(out + offset, list->segments[i], size * sizeof(SEGMENTLIST_TYPE));
    offset += size;
  }

  memcpy(out + offset, list->segments[segment_count-1], list->len - offset);
}
#pragma clang diagnostic pop

#undef SEGMENTLIST_OUTPUT_DEFINITIONS
#endif // SEGMENTLIST_OUTPUT_DEFINITIONS

#undef SEGMENTLIST_NAME
#undef SEGMENTLIST_TYPE
#undef SEGMENTLIST_FUNCTION_PREFIX
#undef SEGMENTLIST_MIN_SIZE_LOG2
#undef SEGMENTLIST_SEGMENT_COUNT
#undef SEGMENTLIST_LINKAGE
