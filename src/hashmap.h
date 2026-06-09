#ifndef HASHMAP_NAME
#error "'HASHMAP_NAME' must be defined"
#endif

#ifndef HASHMAP_KEY_TYPE
#error "'HASHMAP_KEY_TYPE' must be defined"
#endif

#ifndef HASHMAP_VALUE_TYPE
#error "'HASHMAP_VALUE_TYPE' must be defined"
#endif

#ifndef HASHMAP_FUNCTION_PREFIX
#define HASHMAP_FUNCTION_PREFIX HASHMAP_NAME
#endif

#include "toteload.h"

#define HASHMAP_BUCKET_NAME Cat(HASHMAP_NAME, Bucket)

#ifdef HASHMAP_OUTPUT_TYPES

typedef struct HASHMAP_BUCKET_NAME {
  HASHMAP_KEY_TYPE   key;
  HASHMAP_VALUE_TYPE val;
} HASHMAP_BUCKET_NAME;

typedef struct HASHMAP_NAME {
  Allocator allocator;
  u32 *meta;
  void *context;
  HASHMAP_BUCKET_NAME *buckets;
  u32 mask;
  u32 item_count;
} HASHMAP_NAME;

#ifndef HASHMAP_H
#define HASHMAP_H

typedef struct HashMapOptions {
  Allocator allocator;
  u32       initial_size;
  void     *context;
} HashMapOptions;

#endif // HASHMAP_H

#undef HASHMAP_OUTPUT_TYPES
#endif // HASHMAP_OUTPUT_TYPES

#if defined(HASHMAP_OUTPUT_DECLARATIONS) || defined(HASHMAP_OUTPUT_DEFINITIONS)

void                 Cat(HASHMAP_FUNCTION_PREFIX, _init)(HASHMAP_NAME *map, HashMapOptions *options);
void                 Cat(HASHMAP_FUNCTION_PREFIX, _deinit)(HASHMAP_NAME *map);
u32                  Cat(HASHMAP_FUNCTION_PREFIX, _cap)(HASHMAP_NAME *map);
HASHMAP_BUCKET_NAME *Cat(HASHMAP_FUNCTION_PREFIX, _insert_key_and_get_bucket)(HASHMAP_NAME *map, HASHMAP_KEY_TYPE key, b32 *was_occupied);
HASHMAP_BUCKET_NAME *Cat(HASHMAP_FUNCTION_PREFIX, _remove_key_and_get_bucket)(HASHMAP_NAME *map, HASHMAP_KEY_TYPE key);
HASHMAP_VALUE_TYPE  *Cat(HASHMAP_FUNCTION_PREFIX, _find)(HASHMAP_NAME *map, HASHMAP_KEY_TYPE key);
b32                  Cat(HASHMAP_FUNCTION_PREFIX, _insert)(HASHMAP_NAME *map, HASHMAP_KEY_TYPE key, HASHMAP_VALUE_TYPE value);
b32                  Cat(HASHMAP_FUNCTION_PREFIX, _remove)(HASHMAP_NAME *map, HASHMAP_KEY_TYPE key);

#undef HASHMAP_OUTPUT_DECLARATIONS
#endif // HASHMAP_OUTPUT_DECLARATIONS

#ifdef HASHMAP_OUTPUT_DEFINITIONS

#ifndef HASHMAP_HASH_FN
#error "'HASHMAP_HASH_FN' must be defined"
#endif

#ifndef HASHMAP_KEY_COMPARE_FN
#error "'HASHMAP_KEY_COMPARE_FN' must be defined"
#endif


// 30 bits, 30 msb of hash
// 1 bit, is_tombstone
// 1 bit, is_occupied

// is_occupied is 1 if the entry holds a value.
// is_tombstone is 1 if the entry does not hold a value, but a tombstone

#define Mask_is_occupied  0x00000001
#define Mask_is_tombstone 0x00000002
#define Mask_is_stale     0x00000003
#define Mask_fingerprint  0xfffffffc

#define Min_map_size     4
#define Max_search_depth 32
#define Index_not_found  UINT32_MAX
#define Max_load_factor  0.8

internal always_inline b32 slot_is_empty(u32 meta)     { return meta == 0; }
internal always_inline b32 slot_is_occupied(u32 meta)  { return (meta & Mask_is_occupied) != 0; }
internal always_inline b32 slot_is_tombstone(u32 meta) { return (meta & Mask_is_tombstone) != 0; }
internal always_inline b32 slot_is_stale(u32 meta)     { return (meta & Mask_is_stale) == Mask_is_stale; }
internal always_inline u32 read_fingerprint(u32 x)     { return x & Mask_fingerprint; }

internal void Cat(HASHMAP_FUNCTION_PREFIX, __grow_and_rehash)(HASHMAP_NAME *map);
internal u32  Cat(HASHMAP_FUNCTION_PREFIX, __find_occupied_index)(HASHMAP_NAME *map, HASHMAP_KEY_TYPE key);
internal u32  Cat(HASHMAP_FUNCTION_PREFIX, __find_insert_index)(HASHMAP_NAME *map, u32 hash, HASHMAP_KEY_TYPE key);
internal b32  Cat(HASHMAP_FUNCTION_PREFIX, __rehash)(HASHMAP_NAME *map, void *mem, u32 cap, u32 size);

void Cat(HASHMAP_FUNCTION_PREFIX, _init)(HASHMAP_NAME *map, HashMapOptions *options) {
  u32 size = Max(options->initial_size, Min_map_size);

  Assert(is_zero_or_power_of_two(size));

  u32 byte_size = size * (sizeof(u32) + sizeof(HASHMAP_BUCKET_NAME));

  HASHMAP_BUCKET_NAME *buckets = Alloc(options->allocator, byte_size, Align_of(HASHMAP_BUCKET_NAME));
  u32 *meta = Cast(u32*, buckets + size);

  memset(meta, 0, size * sizeof(u32));

  map->allocator  = options->allocator;
  map->buckets    = buckets;
  map->meta       = meta;
  map->context    = options->context;
  map->mask       = size - 1;
  map->item_count = 0;
}

void Cat(HASHMAP_FUNCTION_PREFIX, _deinit)(HASHMAP_NAME *map) {
  if (!is_null(map->buckets)) {
    u32 cap = Cat(HASHMAP_FUNCTION_PREFIX, _cap)(map);
    u32 byte_size = cap * (sizeof(HASHMAP_BUCKET_NAME) + sizeof(u32));
    Free(map->allocator, map->buckets, byte_size);
  }

  memset(map, 0, sizeof(*map));
}

u32 Cat(HASHMAP_FUNCTION_PREFIX, _cap)(HASHMAP_NAME *map) {
  return map->mask + 1;
}

HASHMAP_BUCKET_NAME *Cat(HASHMAP_FUNCTION_PREFIX, _insert_key_and_get_bucket)(HASHMAP_NAME *map, HASHMAP_KEY_TYPE key, b32 *was_occupied) {
  // This load factor check has a potential weakness, because it doesn't take tombstones into account.
  // If you repeatedly insert and remove the same keys, then you could end up with chains of tombstones.
  // As a consequence you could end up with occupied buckets at the end of long chains resulting in slow lookups.
  // Although I am not sure how likely this scenario is to occur.

  u32 cap = Cat(HASHMAP_FUNCTION_PREFIX, _cap)(map);
  if (map->item_count > cap * Max_load_factor) {
    Cat(HASHMAP_FUNCTION_PREFIX, __grow_and_rehash)(map);
  }

  u32 hash = HASHMAP_HASH_FN(map->context, key);
  u32 idx = Cat(HASHMAP_FUNCTION_PREFIX, __find_insert_index)(map, hash, key);

  while (idx == Index_not_found) {
    Cat(HASHMAP_FUNCTION_PREFIX, __grow_and_rehash)(map);
    idx = Cat(HASHMAP_FUNCTION_PREFIX, __find_insert_index)(map, hash, key);
  }

  b32 is_occupied = slot_is_occupied(map->meta[idx]);

  if (!is_occupied) {
    map->item_count += 1;
    map->meta[idx] = read_fingerprint(hash) | Mask_is_occupied;
    map->buckets[idx].key = key;
  }

  *was_occupied = is_occupied;

  return &map->buckets[idx];
}

// Returns Null if the item was not present in the map, otherwise returns a pointer to the bucket of the removed item.
HASHMAP_BUCKET_NAME *Cat(HASHMAP_FUNCTION_PREFIX , _remove_key_and_get_bucket)(HASHMAP_NAME *map, HASHMAP_KEY_TYPE key) {
  u32 idx = Cat(HASHMAP_FUNCTION_PREFIX, __find_occupied_index)(map, key);
  if (idx == Index_not_found) {
    return Null;
  }

  map->item_count -= 1;

  map->meta[idx] = Mask_is_tombstone;

  return &map->buckets[idx];
}

HASHMAP_VALUE_TYPE *Cat(HASHMAP_FUNCTION_PREFIX, _find)(HASHMAP_NAME *map, HASHMAP_KEY_TYPE key) {
  u32 idx = Cat(HASHMAP_FUNCTION_PREFIX, __find_occupied_index)(map, key);
  if (idx == Index_not_found) {
    return Null;
  }

  return &map->buckets[idx].val;
}

b32 Cat(HASHMAP_FUNCTION_PREFIX, _insert)(HASHMAP_NAME *map, HASHMAP_KEY_TYPE key, HASHMAP_VALUE_TYPE value) {
  b32 was_occupied;
  HASHMAP_BUCKET_NAME *bucket = Cat(HASHMAP_FUNCTION_PREFIX, _insert_key_and_get_bucket)(map, key, &was_occupied);

  bucket->val = value;

  return was_occupied;
}

b32 Cat(HASHMAP_FUNCTION_PREFIX, _remove)(HASHMAP_NAME *map, HASHMAP_KEY_TYPE key) {
  HASHMAP_BUCKET_NAME *bucket = Cat(HASHMAP_FUNCTION_PREFIX, _remove_key_and_get_bucket)(map, key);
  return !is_null(bucket);
}

internal void Cat(HASHMAP_FUNCTION_PREFIX, __grow_and_rehash)(HASHMAP_NAME *map) {
  u32 cap = Cat(HASHMAP_FUNCTION_PREFIX, _cap)(map);
  u32 size = cap * 2;

  while (True) {
    if (size == 0) { Panic(); }

    usize new_byte_size = size * (sizeof(u32) + sizeof(HASHMAP_BUCKET_NAME));

    void *mem = Alloc(map->allocator, new_byte_size, Align_of(HASHMAP_BUCKET_NAME));

    b32 ok = Cat(HASHMAP_FUNCTION_PREFIX, __rehash)(map, mem, cap, size);
    if (ok) {
      usize old_byte_size = cap * (sizeof(u32) + sizeof(HASHMAP_BUCKET_NAME));

      Free(map->allocator, map->buckets, old_byte_size);

      HASHMAP_BUCKET_NAME *buckets = mem;
      u32 *meta = Cast(u32*, buckets + size);

      map->mask    = size - 1;
      map->meta    = meta;
      map->buckets = buckets;

      break;
    }

    Free(map->allocator, mem, new_byte_size);

    size *= 2;
  }
}

internal b32 Cat(HASHMAP_FUNCTION_PREFIX, __rehash)(HASHMAP_NAME *map, void *mem, u32 cap, u32 size) {
  HASHMAP_BUCKET_NAME *buckets = mem;
  u32 *meta = Cast(u32*, buckets + size);

  memcpy(buckets, map->buckets, cap * sizeof(HASHMAP_BUCKET_NAME));
  memcpy(meta, map->meta, cap * sizeof(u32));
  memset(meta + cap, 0, (size - cap) * sizeof(u32));

  // Mark occupied slots as stale and remove tombstones.
  for EachIndex(i, cap) {
    if (slot_is_occupied(meta[i])) {
      meta[i] |= Mask_is_stale;
    } else if (slot_is_tombstone(meta[i])) {
      meta[i] = 0;
    }
  }

  u32 mask = size - 1;

  for EachIndex(i, cap) {
    if (!slot_is_stale(meta[i])) {
      continue;
    }

    HASHMAP_BUCKET_NAME bi = buckets[i];

    u32 hash = HASHMAP_HASH_FN(map->context, bi.key);
    u32 start = hash & mask;

    u32 idx = Index_not_found;
    for EachIndex(k, Max_search_depth) {
      u32 j = (start + k) & mask;
      if (slot_is_empty(meta[j]) || slot_is_stale(meta[j])) {
        idx = j;
        break;
      }
    }

    if (idx == Index_not_found) {
      return False;
    }

    if (i == idx) {
      meta[idx] = read_fingerprint(hash) | Mask_is_occupied;
      continue;
    }

    meta[i] = 0;

    if (slot_is_empty(meta[idx])) {
      meta[idx] = read_fingerprint(hash) | Mask_is_occupied;
      buckets[idx] = bi;
      continue;
    }

    if (slot_is_stale(meta[idx])) {
      meta[idx] = read_fingerprint(hash) | Mask_is_occupied;
      Swap(HASHMAP_BUCKET_NAME, bi, buckets[idx]);
    }

    for EachIndex(j, cap) {
      u32 hash = HASHMAP_HASH_FN(map->context, bi.key);
      u32 start = hash & mask;

      for EachIndex(k, Max_search_depth) {
        u32 idx = (start + k) & mask;

        if (slot_is_empty(meta[idx])) {
          meta[idx] = read_fingerprint(hash) | Mask_is_occupied;
          buckets[idx] = bi;
          goto next;
        }

        if (slot_is_stale(meta[idx])) {
          meta[idx] = read_fingerprint(hash) | Mask_is_occupied;
          Swap(HASHMAP_BUCKET_NAME, bi, buckets[idx]);
          goto place_next_bucket_in_chain;
        }
      }

      return False;

    place_next_bucket_in_chain:
      continue;
    }

  next:
    continue;
  }

  return True;
}

internal u32 Cat(HASHMAP_FUNCTION_PREFIX, __find_occupied_index)(HASHMAP_NAME *map, HASHMAP_KEY_TYPE key) {
  u32 hash = HASHMAP_HASH_FN(map->context, key);
  u32 fingerprint = read_fingerprint(hash);
  u32 start_idx = hash & map->mask;

  for EachIndex(j, Max_search_depth) {
    u32 idx = (start_idx + j) & map->mask;

    if (slot_is_empty(map->meta[idx])) {
      return Index_not_found;
    }

    if (slot_is_occupied(map->meta[idx]) && read_fingerprint(map->meta[idx]) == fingerprint) {
      if (HASHMAP_KEY_COMPARE_FN(map->context, key, map->buckets[idx].key)) {
        return idx;
      }
    }
  }

  return Index_not_found;
}

internal u32 Cat(HASHMAP_FUNCTION_PREFIX, __find_insert_index)(HASHMAP_NAME *map, u32 hash, HASHMAP_KEY_TYPE key) {
  u32 fingerprint = read_fingerprint(hash);
  u32 start_idx = hash & map->mask;

  u32 tombstone_idx = Index_not_found;

  for EachIndex(j, Max_search_depth) {
    u32 idx = (start_idx + j) & map->mask;

    if (slot_is_empty(map->meta[idx])) {
      if (tombstone_idx != Index_not_found) {
        return tombstone_idx;
      } else {
        return idx;
      }
    }

    if (slot_is_tombstone(map->meta[idx]) && tombstone_idx == Index_not_found) {
      tombstone_idx = idx;
    }

    if (slot_is_occupied(map->meta[idx]) && read_fingerprint(map->meta[idx]) == fingerprint) {
      if (HASHMAP_KEY_COMPARE_FN(map->context, key, map->buckets[idx].key)) {
        return idx;
      }
    }
  }

  return tombstone_idx;
}

#undef HASHMAP_OUTPUT_DEFINITIONS
#endif // HASHMAP_OUTPUT_DEFINITIONS

#undef HASHMAP_NAME
#undef HASHMAP_FUNCTION_PREFIX
#undef HASHMAP_KEY_TYPE
#undef HASHMAP_VALUE_TYPE
#undef HASHMAP_HASH_FN
#undef HASHMAP_KEY_COMPARE_FN
#undef HASHMAP_BUCKET_NAME

#undef Mask_is_occupied
#undef Mask_is_tombstone
#undef Mask_is_stale
#undef Mask_fingerprint
#undef Min_map_size
#undef Max_search_depth
#undef Index_not_found
#undef Max_load_factor
