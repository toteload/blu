#include "toteload.h"
#include <stdlib.h>
#include <stdio.h>

u32 hash_i32(void *hash_context, i32 x) {
  Unused(hash_context);

  return Cast(u32, x);
}

b32 cmp_i32(i32 a, i32 b) {
  return a == b;
}

#define HASHMAP_NAME            IntMap
#define HASHMAP_FUNCTION_PREFIX map
#define HASHMAP_KEY_TYPE        i32
#define HASHMAP_VALUE_TYPE      i32
#define HASHMAP_HASH_FN         hash_i32
#define HASHMAP_KEY_COMPARE_FN  cmp_i32
#define HASHMAP_OUTPUT_DEFINITIONS
#include "hashmap.h"

void *std_alloc(void *ctx, void *ptr, size_t old_byte_size, size_t new_byte_size, u32 align) {
  Unused(ctx);
  Unused(old_byte_size);
  Unused(align);

  if (!Is_null(ptr) && new_byte_size == 0) {
    free(ptr);
    return Null;
  }

  if (new_byte_size == 0) {
    return Null;
  }

  return realloc(ptr, new_byte_size);
}

i32 main() {
  Allocator allocator = { .fn = std_alloc, };

  IntMap map = {};
  map_init(&map, &(HashMapOptions){ .allocator = allocator, });

  map_insert(&map, 0, 1);
  map_insert(&map, 1, 2);
  map_insert(&map, 2, 4);
  map_insert(&map, 3, 8);
  map_insert(&map, 4, 16);
  map_insert(&map, 8, 99999);

  Assert(map.item_count == 6);

  i32 *p = Null;

  // Item that is present in map should be found.
  p = map_find(&map, 2);
  Assert(!Is_null(p));
  Assert(*p == 4);

  // Item that is present in map should be found.
  p = map_find(&map, 8);
  Assert(!Is_null(p));
  Assert(*p == 99999);

  // Item that is absent in map should not found.
  p = map_find(&map, 5);
  Assert(Is_null(p));

  b32 was_removed = False;

  // Item that is present in map should be removed.
  was_removed = map_remove(&map, 2);
  Assert(was_removed);

  // Item that was removed should be able to be removed again.
  p = map_find(&map, 2);
  Assert(Is_null(p));

  // Item that was never in map should not be able to be removed.
  was_removed = map_remove(&map, 7);
  Assert(!was_removed);

  Assert(map.item_count == 5);

  // Item that is present in map should be removed.
  was_removed = map_remove(&map, 4);
  Assert(was_removed);

  b32 was_occupied = False;

  // Item that was previously removed should leave hole.
  was_occupied = map_insert(&map, 4, 123);
  Assert(!was_occupied);

  printf("ok\n");

  return 0;
}
