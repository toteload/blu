#include "string_interner.h"

#define SEGMENTLIST_NAME StringList
#define SEGMENTLIST_TYPE String
#define SEGMENTLIST_MIN_SIZE_LOG2 6
#define SEGMENTLIST_SEGMENT_COUNT 24
#define SEGMENTLIST_FUNCTION_PREFIX list
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

#define XXH_INLINE_ALL
#include "xxhash.h"

u32 str_hash(void *context, String s) { 
  Unused(context);
  return XXH32(s.str, s.len, 0);
}

b32 str_eq(void *context, String a, String b) {
  Unused(context);
  return string_eq(a, b);
}

#define HASHMAP_NAME            StringIndexMap
#define HASHMAP_FUNCTION_PREFIX map
#define HASHMAP_KEY_TYPE        String
#define HASHMAP_VALUE_TYPE      StringIndex
#define HASHMAP_HASH_FN         str_hash
#define HASHMAP_KEY_COMPARE_FN  str_eq
#define HASHMAP_OUTPUT_DEFINITIONS
#include "hashmap.h"

void strings_init(StringInterner *strings, StringInternerOptions *options) {
  strings->arena = options->arena;
  map_init(&strings->map, &(HashMapOptions){ .allocator = options->map_allocator, });
  strings->list = (StringList){0};
}

void strings_deinit(StringInterner *strings) {
  map_deinit(&strings->map);
  memset(strings, 0, sizeof(StringInterner));
}

StringIndex strings_add(StringInterner *strings, String s) {
  b32 was_occupied;
  StringIndexMapBucket *bucket = map_insert_key_and_get_bucket(&strings->map, s, &was_occupied);

  if (was_occupied) {
    return bucket->val;
  }

  u8 *mem = arena_push(strings->arena, s.len, 1);
  memcpy(mem, s.str, s.len);

  String intern = { .str = mem, .len = s.len, };
  u32 idx = strings->list.len;

  *bucket = (StringIndexMapBucket){ .key = intern, .val = idx, }; 

  list_append(&strings->list, strings->arena, intern);

  return idx;
}

String strings_get(StringInterner *strings, StringIndex idx) {
  return *list_ptr_at_unchecked(&strings->list, idx);
}
