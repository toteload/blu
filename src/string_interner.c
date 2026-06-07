#include "string_interner.h"

#define VECTOR_NAME            StringVector
#define VECTOR_FUNCTION_PREFIX vec
#define VECTOR_TYPE            String
#include "vector.h"

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
#define HASHMAP_VALUE_TYPE      u32
#define HASHMAP_HASH_FN         str_hash
#define HASHMAP_KEY_COMPARE_FN  str_eq
#define HASHMAP_OUTPUT_DEFINITIONS
#include "hash_map.h"

void string_interner_init(StringInterner *strings, StringInternerOptions *options) {
  strings->storage = options->string_storage;
  map_init(&strings->map, &(HashMapOptions){ .allocator = options->map_allocator, });
  vec_init(&strings->vec, options->vec_allocator);
}

void string_interner_deinit(StringInterner *strings) {
  map_deinit(&strings->map);
  vec_deinit(&strings->vec);
  memset(strings, 0, sizeof(StringInterner));
}

u32 string_interner_add(StringInterner *strings, String s) {
  b32 was_occupied;
  StringIndexMapBucket *bucket = map_insert_key_and_get_bucket(&strings->map, s, &was_occupied);

  if (was_occupied) {
    return bucket->val;
  }

  u8 *mem = arena_push(&strings->storage, s.len, 1);
  memcpy(mem, s.str, s.len);

  String intern = { .str = mem, .len = s.len, };
  u32 idx = strings->vec.len;

  *bucket = { .key = intern, .val = idx, }; 

  vec_push(&strings->vec, intern);

  return idx;
}

String string_interner_get(StringInterner *strings, u32 key) {
  return strings->vec.data[key];
}
