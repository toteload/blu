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

#ifndef HASHMAP_LINKAGE
#define HASHMAP_LINKAGE
#endif

#include "toteload.h"

#ifndef HASHMAP_BUCKET_NAME
#define HASHMAP_BUCKET_NAME Cat(HASHMAP_NAME, Bucket)
#endif // HASHMAP_BUCKET_NAME

#ifdef HASHMAP_OUTPUT_TYPES

typedef struct HASHMAP_BUCKET_NAME {
  HASHMAP_KEY_TYPE   key;
  HASHMAP_VALUE_TYPE val;
} HASHMAP_BUCKET_NAME;

typedef struct HASHMAP_NAME {
  Allocator allocator;
  void *context;
  u8 *meta;
  HASHMAP_BUCKET_NAME *buckets;
  u32 mask;
  u32 item_count;
  u32 tombstone_count;
} HASHMAP_NAME;

#ifndef HASHMAP_H
#define HASHMAP_H

typedef struct {
  Allocator allocator;
  u32       initial_size;
  void     *context;
} HashMapOptions;

#endif // HASHMAP_H

#undef HASHMAP_OUTPUT_TYPES
#endif // HASHMAP_OUTPUT_TYPES

#if defined(HASHMAP_OUTPUT_DECLARATIONS) || defined(HASHMAP_OUTPUT_DEFINITIONS)

HASHMAP_LINKAGE void                 Cat(HASHMAP_FUNCTION_PREFIX, _init)(HASHMAP_NAME *map, HashMapOptions *options);
HASHMAP_LINKAGE void                 Cat(HASHMAP_FUNCTION_PREFIX, _deinit)(HASHMAP_NAME *map);
HASHMAP_LINKAGE u32                  Cat(HASHMAP_FUNCTION_PREFIX, _cap)(HASHMAP_NAME *map);
HASHMAP_LINKAGE HASHMAP_BUCKET_NAME *Cat(HASHMAP_FUNCTION_PREFIX, _insert_key_and_get_bucket)(HASHMAP_NAME *map, HASHMAP_KEY_TYPE key, b32 *was_occupied);
HASHMAP_LINKAGE HASHMAP_BUCKET_NAME *Cat(HASHMAP_FUNCTION_PREFIX, _remove_key_and_get_bucket)(HASHMAP_NAME *map, HASHMAP_KEY_TYPE key);
HASHMAP_LINKAGE HASHMAP_VALUE_TYPE  *Cat(HASHMAP_FUNCTION_PREFIX, _find)(HASHMAP_NAME *map, HASHMAP_KEY_TYPE key);
HASHMAP_LINKAGE b32                  Cat(HASHMAP_FUNCTION_PREFIX, _insert)(HASHMAP_NAME *map, HASHMAP_KEY_TYPE key, HASHMAP_VALUE_TYPE value);
HASHMAP_LINKAGE b32                  Cat(HASHMAP_FUNCTION_PREFIX, _remove)(HASHMAP_NAME *map, HASHMAP_KEY_TYPE key);
HASHMAP_LINKAGE b32                  Cat(HASHMAP_FUNCTION_PREFIX, _contains_key)(HASHMAP_NAME *map, HASHMAP_KEY_TYPE key);

#undef HASHMAP_OUTPUT_DECLARATIONS
#endif // HASHMAP_OUTPUT_DECLARATIONS

#ifdef HASHMAP_OUTPUT_DEFINITIONS
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"

#ifndef HASHMAP_HASH_FN
#error "'HASHMAP_HASH_FN' must be defined"
#endif

#ifndef HASHMAP_KEY_COMPARE_FN
#error "'HASHMAP_KEY_COMPARE_FN' must be defined"
#endif

#define Meta_empty     0x00
#define Meta_tombstone 0x01
#define Meta_stale     0x02
#define Meta_occupied  0x80

#define Group_size       16u
#define Min_map_size     Group_size
#define Index_not_found  UINT32_MAX
#define Max_load_factor  0.8

#define Buckets_align Max(Group_size, Align_of(HASHMAP_BUCKET_NAME))

#ifndef HASHMAP_INTERNAL_FUNCTIONS_DEFINED
#define HASHMAP_INTERNAL_FUNCTIONS_DEFINED

#if defined(__SSE2__)
#include <emmintrin.h>
#endif

internal always_inline b32 slot_is_empty(u8 meta)     { return meta == Meta_empty; }
internal always_inline b32 slot_is_occupied(u8 meta)  { return (meta & Meta_occupied) != 0; }
internal always_inline b32 slot_is_tombstone(u8 meta) { return meta == Meta_tombstone; }
internal always_inline b32 slot_is_stale(u8 meta)     { return meta == Meta_stale; }
internal always_inline u8  occupied_meta(u32 hash)    { return Cast(u8, Meta_occupied | (hash >> 25)); }

#if defined(__SSE2__)

internal always_inline u32 group_match(u8 const *group, u8 value) {
  __m128i g = _mm_load_si128(Cast(__m128i const*, group));
  return Cast(u32, _mm_movemask_epi8(_mm_cmpeq_epi8(g, _mm_set1_epi8(Cast(char, value)))));
}

internal always_inline u32 group_match_unoccupied(u8 const *group) {
  __m128i g = _mm_load_si128(Cast(__m128i const*, group));
  return ~Cast(u32, _mm_movemask_epi8(g)) & 0xffff;
}

#else

internal always_inline u32 group_match(u8 const *group, u8 value) {
  u32 mask = 0;
  for (u32 i = 0; i < Group_size; i++) {
    mask |= Cast(u32, group[i] == value) << i;
  }
  return mask;
}

internal always_inline u32 group_match_unoccupied(u8 const *group) {
  u32 mask = 0;
  for (u32 i = 0; i < Group_size; i++) {
    mask |= Cast(u32, !slot_is_occupied(group[i])) << i;
  }
  return mask;
}

#endif

#endif // HASHMAP_INTERNAL_FUNCTIONS_DEFINED

internal void Cat(HASHMAP_FUNCTION_PREFIX, __grow_and_rehash)(HASHMAP_NAME *map);
internal void Cat(HASHMAP_FUNCTION_PREFIX, __rehash_in_place)(HASHMAP_NAME *map);
internal u32  Cat(HASHMAP_FUNCTION_PREFIX, __find_occupied_index)(HASHMAP_NAME *map, HASHMAP_KEY_TYPE key);
internal u32  Cat(HASHMAP_FUNCTION_PREFIX, __find_insert_index)(HASHMAP_NAME *map, u32 hash, HASHMAP_KEY_TYPE key);
internal u32  Cat(HASHMAP_FUNCTION_PREFIX, __find_unoccupied_index)(HASHMAP_NAME *map, u32 hash);

#define Buckets_offset(cap) round_up_to_power_of_two(cap, Buckets_align)
#define Map_byte_size(cap) (Buckets_offset(cap) + cap * sizeof(HASHMAP_BUCKET_NAME))

HASHMAP_LINKAGE 
void Cat(HASHMAP_FUNCTION_PREFIX, _init)(HASHMAP_NAME *map, HashMapOptions *options) {
  u32 size = Max(options->initial_size, Min_map_size);

  Assert(is_zero_or_power_of_two(size));

  usize byte_size = Map_byte_size(size);

  u8 *meta = Alloc(options->allocator, byte_size, Buckets_align);
  Assert(ptr_forward_align(meta, Buckets_align) == meta);

  memset(meta, Meta_empty, size);

  map->allocator       = options->allocator;
  map->meta            = meta;
  map->buckets         = Cast(HASHMAP_BUCKET_NAME*, meta + Buckets_offset(size));
  map->context         = options->context;
  map->mask            = size - 1;
  map->item_count      = 0;
  map->tombstone_count = 0;
}

HASHMAP_LINKAGE 
void Cat(HASHMAP_FUNCTION_PREFIX, _deinit)(HASHMAP_NAME *map) {
  if (!is_null(map->meta)) {
    u32 cap = Cat(HASHMAP_FUNCTION_PREFIX, _cap)(map);
    Free(map->allocator, map->meta, Map_byte_size(cap));
  }

  memset(map, 0, sizeof(*map));
}

HASHMAP_LINKAGE 
u32 Cat(HASHMAP_FUNCTION_PREFIX, _cap)(HASHMAP_NAME *map) {
  return map->mask + 1;
}

HASHMAP_LINKAGE 
HASHMAP_BUCKET_NAME *Cat(HASHMAP_FUNCTION_PREFIX, _insert_key_and_get_bucket)(HASHMAP_NAME *map, HASHMAP_KEY_TYPE key, b32 *was_occupied) {
  u32 cap = Cat(HASHMAP_FUNCTION_PREFIX, _cap)(map);
  if (map->item_count + map->tombstone_count + 1 > cap * Max_load_factor) {
    if (map->item_count * 2 < cap) {
      Cat(HASHMAP_FUNCTION_PREFIX, __rehash_in_place)(map);
    } else {
      Cat(HASHMAP_FUNCTION_PREFIX, __grow_and_rehash)(map);
    }
  }

  u32 hash = HASHMAP_HASH_FN(map->context, key);
  u32 idx = Cat(HASHMAP_FUNCTION_PREFIX, __find_insert_index)(map, hash, key);

  b32 is_occupied = slot_is_occupied(map->meta[idx]);

  if (!is_occupied) {
    if (slot_is_tombstone(map->meta[idx])) {
      map->tombstone_count -= 1;
    }

    map->item_count += 1;
    map->meta[idx] = occupied_meta(hash);
    map->buckets[idx].key = key;
  }

  *was_occupied = is_occupied;

  return &map->buckets[idx];
}

// Returns Null if the item was not present in the map, otherwise returns a pointer to the bucket of the removed item.
HASHMAP_LINKAGE 
HASHMAP_BUCKET_NAME *Cat(HASHMAP_FUNCTION_PREFIX , _remove_key_and_get_bucket)(HASHMAP_NAME *map, HASHMAP_KEY_TYPE key) {
  u32 idx = Cat(HASHMAP_FUNCTION_PREFIX, __find_occupied_index)(map, key);
  if (idx == Index_not_found) {
    return Null;
  }

  map->item_count      -= 1;
  map->tombstone_count += 1;

  map->meta[idx] = Meta_tombstone;

  return &map->buckets[idx];
}

HASHMAP_LINKAGE 
HASHMAP_VALUE_TYPE *Cat(HASHMAP_FUNCTION_PREFIX, _find)(HASHMAP_NAME *map, HASHMAP_KEY_TYPE key) {
  u32 idx = Cat(HASHMAP_FUNCTION_PREFIX, __find_occupied_index)(map, key);
  if (idx == Index_not_found) {
    return Null;
  }

  return &map->buckets[idx].val;
}

HASHMAP_LINKAGE 
b32 Cat(HASHMAP_FUNCTION_PREFIX, _insert)(HASHMAP_NAME *map, HASHMAP_KEY_TYPE key, HASHMAP_VALUE_TYPE value) {
  b32 was_occupied;
  HASHMAP_BUCKET_NAME *bucket = Cat(HASHMAP_FUNCTION_PREFIX, _insert_key_and_get_bucket)(map, key, &was_occupied);

  bucket->val = value;

  return was_occupied;
}

HASHMAP_LINKAGE 
b32 Cat(HASHMAP_FUNCTION_PREFIX, _remove)(HASHMAP_NAME *map, HASHMAP_KEY_TYPE key) {
  HASHMAP_BUCKET_NAME *bucket = Cat(HASHMAP_FUNCTION_PREFIX, _remove_key_and_get_bucket)(map, key);
  return !is_null(bucket);
}


internal void Cat(HASHMAP_FUNCTION_PREFIX, __grow_and_rehash)(HASHMAP_NAME *map) {
  u32 cap = Cat(HASHMAP_FUNCTION_PREFIX, _cap)(map);
  u32 size = cap * 2;

#ifdef HASHMAP_GROW_WITH_ALLOC
  u8 *old_meta = map->meta;
  HASHMAP_BUCKET_NAME *old_buckets = map->buckets;

  u8 *meta = Alloc(map->allocator, Map_byte_size(size), Buckets_align);
  Assert(ptr_forward_align(meta, Buckets_align) == meta);

  memset(meta, Meta_empty, size);

  map->meta            = meta;
  map->buckets         = Cast(HASHMAP_BUCKET_NAME*, meta + Buckets_offset(size));
  map->mask            = size - 1;
  map->tombstone_count = 0;

  for (u32 i = 0; i < cap; i++) {
    if (!slot_is_occupied(old_meta[i])) {
      continue;
    }

    u32 hash = HASHMAP_HASH_FN(map->context, old_buckets[i].key);
    u32 idx = Cat(HASHMAP_FUNCTION_PREFIX, __find_unoccupied_index)(map, hash);

    map->meta[idx]    = old_meta[i];
    map->buckets[idx] = old_buckets[i];
  }

  Free(map->allocator, old_meta, Map_byte_size(cap));
#else
  usize old_offset = Buckets_offset(cap);
  usize new_offset = Buckets_offset(size);

  u8 *meta = Realloc(map->allocator, map->meta, Map_byte_size(cap), Map_byte_size(size), Buckets_align);
  Assert(ptr_forward_align(meta, Buckets_align) == meta);

  memmove(meta + new_offset, meta + old_offset, cap * sizeof(HASHMAP_BUCKET_NAME));
  memset(meta + cap, Meta_empty, cap);

  map->meta    = meta;
  map->buckets = Cast(HASHMAP_BUCKET_NAME*, meta + new_offset);
  map->mask    = size - 1;

  Cat(HASHMAP_FUNCTION_PREFIX, __rehash_in_place)(map);
#endif
}

internal void Cat(HASHMAP_FUNCTION_PREFIX, __rehash_in_place)(HASHMAP_NAME *map) {
  u8 *meta = map->meta;
  HASHMAP_BUCKET_NAME *buckets = map->buckets;
  u32 cap = Cat(HASHMAP_FUNCTION_PREFIX, _cap)(map);

  for (u32 i = 0; i < cap; i++) {
    meta[i] = slot_is_occupied(meta[i]) ? Meta_stale : Meta_empty;
  }

  for (u32 i = 0; i < cap; i++) {
    while (slot_is_stale(meta[i])) {
      u32 hash = HASHMAP_HASH_FN(map->context, buckets[i].key);
      u32 idx = Cat(HASHMAP_FUNCTION_PREFIX, __find_unoccupied_index)(map, hash);

      if (((idx ^ i) & ~(Group_size - 1)) == 0) {
        meta[i] = occupied_meta(hash);
      } else if (slot_is_empty(meta[idx])) {
        meta[idx]    = occupied_meta(hash);
        buckets[idx] = buckets[i];
        meta[i]      = Meta_empty;
      } else {
        meta[idx] = occupied_meta(hash);
        Swap(HASHMAP_BUCKET_NAME, buckets[i], buckets[idx]);
      }
    }
  }

  map->tombstone_count = 0;
}

internal u32 Cat(HASHMAP_FUNCTION_PREFIX, __find_occupied_index)(HASHMAP_NAME *map, HASHMAP_KEY_TYPE key) {
  u32 hash = HASHMAP_HASH_FN(map->context, key);
  u8 fingerprint = occupied_meta(hash);
  u32 group_mask = map->mask & ~(Group_size - 1);

  for (u32 g = hash & group_mask; ; g = (g + Group_size) & group_mask) {
    u8 const *group = map->meta + g;

    for (u32 m = group_match(group, fingerprint); m != 0; m &= m - 1) {
      u32 idx = g + count_trailing_zeros64(m);
      if (HASHMAP_KEY_COMPARE_FN(map->context, key, map->buckets[idx].key)) {
        return idx;
      }
    }

    if (group_match(group, Meta_empty) != 0) {
      return Index_not_found;
    }
  }
}

internal u32 Cat(HASHMAP_FUNCTION_PREFIX, __find_insert_index)(HASHMAP_NAME *map, u32 hash, HASHMAP_KEY_TYPE key) {
  u8 fingerprint = occupied_meta(hash);
  u32 group_mask = map->mask & ~(Group_size - 1);

  u32 insert_idx = Index_not_found;

  for (u32 g = hash & group_mask; ; g = (g + Group_size) & group_mask) {
    u8 const *group = map->meta + g;

    for (u32 m = group_match(group, fingerprint); m != 0; m &= m - 1) {
      u32 idx = g + count_trailing_zeros64(m);
      if (HASHMAP_KEY_COMPARE_FN(map->context, key, map->buckets[idx].key)) {
        return idx;
      }
    }

    u32 unoccupied = group_match_unoccupied(group);
    if (insert_idx == Index_not_found && unoccupied != 0) {
      insert_idx = g + count_trailing_zeros64(unoccupied);
    }

    if (group_match(group, Meta_empty) != 0) {
      return insert_idx;
    }
  }
}

internal u32 Cat(HASHMAP_FUNCTION_PREFIX, __find_unoccupied_index)(HASHMAP_NAME *map, u32 hash) {
  u32 group_mask = map->mask & ~(Group_size - 1);

  for (u32 g = hash & group_mask; ; g = (g + Group_size) & group_mask) {
    u32 unoccupied = group_match_unoccupied(map->meta + g);
    if (unoccupied != 0) {
      return g + count_trailing_zeros64(unoccupied);
    }
  }
}

#undef HASHMAP_OUTPUT_DEFINITIONS
#pragma clang diagnostic pop
#endif // HASHMAP_OUTPUT_DEFINITIONS

#undef HASHMAP_NAME
#undef HASHMAP_FUNCTION_PREFIX
#undef HASHMAP_KEY_TYPE
#undef HASHMAP_VALUE_TYPE
#undef HASHMAP_HASH_FN
#undef HASHMAP_KEY_COMPARE_FN
#undef HASHMAP_GROW_WITH_ALLOC
#undef HASHMAP_BUCKET_NAME
#undef HASHMAP_LINKAGE

#undef Meta_empty
#undef Meta_tombstone
#undef Meta_stale
#undef Meta_occupied
#undef Group_size
#undef Buckets_align
#undef Min_map_size
#undef Index_not_found
#undef Max_load_factor
