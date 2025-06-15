#include "blu.hh"

#define XXH_INLINE_ALL
#include "xxhash.h"

u32 str_hash(void *context, Str s) { return XXH32(s.str, s.len, 0); }

void StringInterner::init(
  Allocator storage_allocator, Allocator map_allocator, Allocator list_allocator
) {
  storage = storage_allocator;
  map.init(map_allocator);
  list.init(list_allocator);
}

void StringInterner::deinit() {
  // TODO Currently we are leaking the storage memory

  map.deinit();
  list.deinit();
}

StrKey StringInterner::add(Str s) {
  b32 was_occupied;
  auto bucket = map.insert_key_and_get_bucket(s, &was_occupied);
  if (was_occupied) {
    return bucket->val;
  }

  char *intern = storage.alloc<char>(s.len);
  memcpy(intern, s.str, s.len);

  Str owns{intern, s.len};
  StrKey key{cast<u32>(list.len)};

  bucket->key = owns;
  bucket->val = key;

  list.push(owns);

  return key;
}
