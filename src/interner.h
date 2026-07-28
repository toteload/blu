// In a header:
//
//   #define INTERNER_NAME       StringInterner
//   #define INTERNER_TYPE       String
//   #define INTERNER_INDEX_TYPE StringIndex // optional, default u32
//   #define INTERNER_OUTPUT_TYPES
//   #include "interner.h"
//
// In one .c file:
//
//   #define INTERNER_NAME            StringInterner
//   #define INTERNER_TYPE            String
//   #define INTERNER_INDEX_TYPE      StringIndex
//   #define INTERNER_FUNCTION_PREFIX strings  // optional, default INTERNER_NAME
//   #define INTERNER_HASH_FN         str_hash // u32 fn(void *context, INTERNER_TYPE item)
//   #define INTERNER_COMPARE_FN      str_eq   // b32 fn(void *context, INTERNER_TYPE a, INTERNER_TYPE b)
//   #define INTERNER_COPY_FN         str_copy // optional, INTERNER_TYPE fn(Arena *arena, INTERNER_TYPE item)
//   #define INTERNER_OUTPUT_DEFINITIONS
//   #include "interner.h"
//
// `INTERNER_COPY_FN` copies an item into the interner's arena on first insertion so the stored
// item outlives the caller's memory (e.g. copying string bytes, or a variable-sized Type through
// a pointer). Without it the item is stored as-is, which is only correct for self-contained values.
//
// The `context` pointer from `InternerOptions` is passed to the hash and compare functions.

#ifndef INTERNER_NAME
#error "'INTERNER_NAME' must be defined"
#endif

#ifndef INTERNER_TYPE
#error "'INTERNER_TYPE' must be defined"
#endif

#ifndef INTERNER_INDEX_TYPE
#define INTERNER_INDEX_TYPE u32
#endif

#ifndef INTERNER_FUNCTION_PREFIX
#define INTERNER_FUNCTION_PREFIX INTERNER_NAME
#endif

#ifndef INTERNER_LINKAGE
#define INTERNER_LINKAGE
#endif

#ifndef INTERNER_MIN_SIZE_LOG2
#define INTERNER_MIN_SIZE_LOG2 6
#endif

#ifndef INTERNER_SEGMENT_COUNT
#define INTERNER_SEGMENT_COUNT 24
#endif

#include "toteload.h"

#define INTERNER_LIST_NAME   Cat(INTERNER_NAME, List)
#define INTERNER_MAP_NAME    Cat(INTERNER_NAME, Map)
#define INTERNER_BUCKET_NAME Cat(INTERNER_MAP_NAME, Bucket)
#define INTERNER_EXTRA_NAME  Cat(INTERNER_NAME, Extra)

#ifdef INTERNER_OUTPUT_TYPES

#ifdef INTERNER_EXTRA_TYPE
typedef struct { INTERNER_TYPE key; INTERNER_EXTRA_TYPE extra; } INTERNER_EXTRA_NAME;
#define SEGMENTLIST_TYPE INTERNER_EXTRA_NAME
#else
#define SEGMENTLIST_TYPE INTERNER_TYPE
#endif

#define SEGMENTLIST_NAME          INTERNER_LIST_NAME
#define SEGMENTLIST_MIN_SIZE_LOG2 INTERNER_MIN_SIZE_LOG2
#define SEGMENTLIST_SEGMENT_COUNT INTERNER_SEGMENT_COUNT
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

#define HASHMAP_NAME       INTERNER_MAP_NAME
#define HASHMAP_KEY_TYPE   INTERNER_TYPE
#define HASHMAP_VALUE_TYPE INTERNER_INDEX_TYPE
#define HASHMAP_OUTPUT_TYPES
#include "hashmap.h"

typedef struct {
  Arena *arena;
  INTERNER_LIST_NAME list;
  INTERNER_MAP_NAME  map;
} INTERNER_NAME;

#ifndef INTERNER_H
#define INTERNER_H

typedef struct {
  // `arena` is used for:
  // - Storing the interned items (through INTERNER_COPY_FN).
  // - Backing memory for the segment list that maps indices to items.
  Arena     *arena;
  Allocator  map_allocator;
  u32        map_initial_size;
  void      *context; // Passed to the hash and compare functions.
} InternerOptions;

#endif // INTERNER_H

#undef INTERNER_OUTPUT_TYPES
#endif // INTERNER_OUTPUT_TYPES

#if defined(INTERNER_OUTPUT_DECLARATIONS) || defined(INTERNER_OUTPUT_DEFINITIONS)

INTERNER_LINKAGE void                Cat(INTERNER_FUNCTION_PREFIX, _init)(INTERNER_NAME *interner, InternerOptions *options);
INTERNER_LINKAGE void                Cat(INTERNER_FUNCTION_PREFIX, _deinit)(INTERNER_NAME *interner);
INTERNER_LINKAGE INTERNER_INDEX_TYPE Cat(INTERNER_FUNCTION_PREFIX, _add)(INTERNER_NAME *interner, INTERNER_TYPE item);
INTERNER_LINKAGE INTERNER_INDEX_TYPE Cat(INTERNER_FUNCTION_PREFIX, _add_checked)(INTERNER_NAME *interner, INTERNER_TYPE item, b32 *already_present);
INTERNER_LINKAGE b32                 Cat(INTERNER_FUNCTION_PREFIX, _find)(INTERNER_NAME *interner, INTERNER_TYPE item, INTERNER_INDEX_TYPE *idx);
INTERNER_LINKAGE INTERNER_TYPE       Cat(INTERNER_FUNCTION_PREFIX, _get)(INTERNER_NAME *interner, INTERNER_INDEX_TYPE idx);

#ifdef INTERNER_EXTRA_TYPE
INTERNER_LINKAGE INTERNER_EXTRA_TYPE  Cat(INTERNER_FUNCTION_PREFIX, _get_extra)(INTERNER_NAME *interner, INTERNER_INDEX_TYPE idx);
INTERNER_LINKAGE INTERNER_EXTRA_TYPE *Cat(INTERNER_FUNCTION_PREFIX, _extra_get_ptr)(INTERNER_NAME *interner, INTERNER_INDEX_TYPE idx);
INTERNER_LINKAGE void                 Cat(INTERNER_FUNCTION_PREFIX, _set_extra)(INTERNER_NAME *interner, INTERNER_INDEX_TYPE idx, INTERNER_EXTRA_TYPE);
#endif

#undef INTERNER_OUTPUT_DECLARATIONS
#endif // INTERNER_OUTPUT_DECLARATIONS

#ifdef INTERNER_OUTPUT_DEFINITIONS

#ifndef INTERNER_HASH_FN
#error "'INTERNER_HASH_FN' must be defined"
#endif

#ifndef INTERNER_COMPARE_FN
#error "'INTERNER_COMPARE_FN' must be defined"
#endif

#define INTERNER_LIST_PREFIX Cat(INTERNER_FUNCTION_PREFIX, __list)
#define INTERNER_MAP_PREFIX  Cat(INTERNER_FUNCTION_PREFIX, __map)

#ifdef INTERNER_EXTRA_TYPE
#define SEGMENTLIST_TYPE INTERNER_EXTRA_NAME
#else
#define SEGMENTLIST_TYPE INTERNER_TYPE
#endif

#define SEGMENTLIST_NAME            INTERNER_LIST_NAME
#define SEGMENTLIST_FUNCTION_PREFIX INTERNER_LIST_PREFIX
#define SEGMENTLIST_MIN_SIZE_LOG2   INTERNER_MIN_SIZE_LOG2
#define SEGMENTLIST_SEGMENT_COUNT   INTERNER_SEGMENT_COUNT
#define SEGMENTLIST_LINKAGE         internal
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

#define HASHMAP_NAME            INTERNER_MAP_NAME
#define HASHMAP_KEY_TYPE        INTERNER_TYPE
#define HASHMAP_VALUE_TYPE      INTERNER_INDEX_TYPE
#define HASHMAP_FUNCTION_PREFIX INTERNER_MAP_PREFIX
#define HASHMAP_HASH_FN         INTERNER_HASH_FN
#define HASHMAP_KEY_COMPARE_FN  INTERNER_COMPARE_FN
#define HASHMAP_LINKAGE         internal
#define HASHMAP_OUTPUT_DEFINITIONS
#include "hashmap.h"

INTERNER_LINKAGE
void Cat(INTERNER_FUNCTION_PREFIX, _init)(INTERNER_NAME *interner, InternerOptions *options) {
  interner->arena = options->arena;
  zero_struct(INTERNER_LIST_NAME, &interner->list);
  Cat(INTERNER_MAP_PREFIX, _init)(&interner->map, &(HashMapOptions){
    .allocator    = options->map_allocator,
    .initial_size = options->map_initial_size,
    .context      = options->context,
  });

#ifdef INTERNER_RESERVE_ZERO_INDEX
  Cat(INTERNER_LIST_PREFIX, _push)(&interner->list, interner->arena);
#endif
}

INTERNER_LINKAGE
void Cat(INTERNER_FUNCTION_PREFIX, _deinit)(INTERNER_NAME *interner) {
  Cat(INTERNER_MAP_PREFIX, _deinit)(&interner->map);
  zero_struct(INTERNER_NAME, interner);
}

INTERNER_LINKAGE
INTERNER_INDEX_TYPE Cat(INTERNER_FUNCTION_PREFIX, _add)(INTERNER_NAME *interner, INTERNER_TYPE item) {
  b32 ignore;
  return Cat(INTERNER_FUNCTION_PREFIX, _add_checked)(interner, item, &ignore);
}

INTERNER_LINKAGE INTERNER_INDEX_TYPE Cat(INTERNER_FUNCTION_PREFIX, _add_checked)(INTERNER_NAME *interner, INTERNER_TYPE item, b32 *already_present) {
  b32 was_occupied;
  INTERNER_BUCKET_NAME *bucket = Cat(INTERNER_MAP_PREFIX, _insert_key_and_get_bucket)(&interner->map, item, &was_occupied);

  if (was_occupied) {
    *already_present = True;
    return bucket->val;
  }

#ifdef INTERNER_COPY_FN
  INTERNER_TYPE intern = INTERNER_COPY_FN(interner->arena, item);
#else
  INTERNER_TYPE intern = item;
#endif

  INTERNER_INDEX_TYPE idx = Cast(INTERNER_INDEX_TYPE, interner->list.len);

  *bucket = (INTERNER_BUCKET_NAME){ .key = intern, .val = idx, };
  *already_present = False;

#ifdef INTERNER_EXTRA_TYPE
  Cat(INTERNER_LIST_PREFIX, _append)(&interner->list, interner->arena, (INTERNER_EXTRA_NAME){ .key = intern });
#else
  Cat(INTERNER_LIST_PREFIX, _append)(&interner->list, interner->arena, intern);
#endif

  return idx;
}

INTERNER_LINKAGE
b32 Cat(INTERNER_FUNCTION_PREFIX, _find)(INTERNER_NAME *interner, INTERNER_TYPE item, INTERNER_INDEX_TYPE *idx) {
  INTERNER_INDEX_TYPE *v = Cat(INTERNER_MAP_PREFIX, _find)(&interner->map, item);
  if (v) {
    *idx = *v;
    return True;
  }

  return False;
}

INTERNER_LINKAGE
INTERNER_TYPE Cat(INTERNER_FUNCTION_PREFIX, _get)(INTERNER_NAME *interner, INTERNER_INDEX_TYPE idx) {
#ifdef INTERNER_EXTRA_TYPE
  return Cat(INTERNER_LIST_PREFIX, _at_unchecked)(&interner->list, idx).key;
#else
  return Cat(INTERNER_LIST_PREFIX, _at_unchecked)(&interner->list, idx);
#endif
}


#ifdef INTERNER_EXTRA_TYPE
INTERNER_LINKAGE
INTERNER_EXTRA_TYPE Cat(INTERNER_FUNCTION_PREFIX, _get_extra)(INTERNER_NAME *interner, INTERNER_INDEX_TYPE idx) {
  return Cat(INTERNER_LIST_PREFIX, _at_unchecked)(&interner->list, idx).extra;
}

INTERNER_LINKAGE
INTERNER_EXTRA_TYPE *Cat(INTERNER_FUNCTION_PREFIX, _extra_get_ptr)(INTERNER_NAME *interner, INTERNER_INDEX_TYPE idx) {
  return &Cat(INTERNER_LIST_PREFIX, _ptr_at_unchecked)(&interner->list, idx)->extra;
}

INTERNER_LINKAGE
void Cat(INTERNER_FUNCTION_PREFIX, _set_extra)(INTERNER_NAME *interner, INTERNER_INDEX_TYPE idx, INTERNER_EXTRA_TYPE val) {
  Cat(INTERNER_LIST_PREFIX, _ptr_at_unchecked)(&interner->list, idx)->extra = val;
}
#endif

#undef INTERNER_OUTPUT_DEFINITIONS
#endif // INTERNER_OUTPUT_DEFINITIONS

#undef INTERNER_NAME
#undef INTERNER_TYPE
#undef INTERNER_INDEX_TYPE
#undef INTERNER_FUNCTION_PREFIX
#undef INTERNER_LINKAGE
#undef INTERNER_MIN_SIZE_LOG2
#undef INTERNER_SEGMENT_COUNT
#undef INTERNER_HASH_FN
#undef INTERNER_COMPARE_FN
#undef INTERNER_COPY_FN
#undef INTERNER_LIST_NAME
#undef INTERNER_MAP_NAME
#undef INTERNER_BUCKET_NAME
#undef INTERNER_LIST_PREFIX
#undef INTERNER_MAP_PREFIX
#undef INTERNER_EXTRA_TYPE
#undef INTERNER_RESERVE_ZERO_INDEX
