#include "toteload.hh"

constexpr u32 min_map_size     = 4;
constexpr u32 max_search_depth = 32;
constexpr u32 index_not_found  = UINT32_MAX;
constexpr f32 max_load_factor  = 0.8f;

template<typename K> using HashFn = u32(void *context, K key);

template<typename K> using KeyCmpFn = b32(void *context, K a, K b);

template<typename K, typename V> struct Bucket {
  K key;
  V val;
};

template<typename K, typename V, KeyCmpFn<K> cmp_key, HashFn<K> hash_key> struct HashMap {
  Allocator alloc;
  u32 len;
  u32 mask;
  u32 *meta;
  Bucket<K, V> *buckets;
  void *context;

  void init(Allocator alloc, void *context = nullptr, u32 size = min_map_size);
  void deinit();

  void insert(K key, V val);
  V *get_ptr(K key) {
    Bucket<K, V> *bucket = get_bucket(key);
    if (bucket == nullptr) {
      return nullptr;
    }

    return &bucket->val;
  }

  V get(K key) { return *get_ptr(key); }
  b32 has(K key) { return get_bucket(key) != nullptr; }

  Bucket<K, V> *insert_key_and_get_bucket(K key, b32 *was_occupied);
  Bucket<K, V> *remove_key(K key);
  Bucket<K, V> *get_bucket(K key);

  void grow_and_rehash(u32 size);
  u32 get_occupied_index(K key);
  u32 get_insert_index(u32 hash, K key);
};

constexpr u32 mask_is_occupied  = 0x00000001;
constexpr u32 mask_is_tombstone = 0x00000002;
constexpr u32 mask_is_stale     = 0x00000003;
constexpr u32 mask_fingerprint  = 0xfffffffc;

ttld_inline b32 is_empty(u32 x) { return x == 0; }

ttld_inline b32 is_occupied(u32 x) { return !!(x & mask_is_occupied); }

ttld_inline b32 has_tombstone(u32 x) { return !!(x & mask_is_tombstone); }

ttld_inline b32 is_stale(u32 x) { return (x & mask_is_stale) == mask_is_stale; }

ttld_inline u32 fingerprint32(u32 hash) { return hash & mask_fingerprint; }

template<typename K, typename V, KeyCmpFn<K> cmp_key, HashFn<K> hash_key>
void HashMap<K, V, cmp_key, hash_key>::init(Allocator alloc, void *context, u32 size) {
  Debug_assert(size != 0 && is_zero_or_power_of_two(size));

  this->alloc   = alloc;
  this->context = context;

  size = max<u32>(4, size);

  u32 byte_size = size * (sizeof(u32) + sizeof(Bucket<K, V>));

  Bucket<K, V> *_buckets = cast<Bucket<K, V> *>(alloc.raw_alloc(byte_size));
  u32 *_meta             = cast<u32 *>(_buckets + size);

  memset(_meta, 0, size * sizeof(u32));

  len     = 0;
  mask    = size - 1;
  buckets = _buckets;
  meta    = _meta;
}

template<typename K, typename V, KeyCmpFn<K> cmp_key, HashFn<K> hash_key>
void HashMap<K, V, cmp_key, hash_key>::deinit() {
  if (buckets != nullptr) {
    u32 cap       = mask + 1;
    u32 byte_size = cap * (sizeof(Bucket<K, V>) + sizeof(u32));
    alloc.free(buckets, byte_size);
  }

  len     = 0;
  mask    = 0;
  meta    = nullptr;
  buckets = nullptr;
}

template<typename K, typename V, KeyCmpFn<K> cmp_key, HashFn<K> hash_key>
void HashMap<K, V, cmp_key, hash_key>::grow_and_rehash(u32 size) {
  u32 cap = mask + 1;
  if (cap >= size) {
    return;
  }

  Debug_assert(size >= min_map_size);
  Debug_assert(is_zero_or_power_of_two(size));

  // The buckets array is stored first in memory followed by the meta array.
  // No padding is needed between the two if the minimum capacity of the hashmap is 4.

  u64 current_size = cap * (sizeof(u32) + sizeof(Bucket<K, V>));
  u64 new_size     = size * (sizeof(u32) + sizeof(Bucket<K, V>));

  Bucket<K, V> *_buckets =
    cast<Bucket<K, V> *>(alloc.raw_realloc(buckets, current_size, new_size, 8));

  u32 *old_meta = cast<u32 *>(ptr_offset(_buckets, cap * sizeof(Bucket<K, V>)));
  u32 *meta     = cast<u32 *>(ptr_offset(_buckets, size * sizeof(Bucket<K, V>)));

  memmove(meta, old_meta, cap * sizeof(u32));
  memset(meta + cap, 0, cap * sizeof(u32));

  // Mark occupied slots as stale and remove tombstones.
  ForEachIndex(i, cap) {
    if (is_occupied(meta[i])) {
      meta[i] |= mask_is_stale;
    } else if (has_tombstone(meta[i])) {
      meta[i] = 0;
    }
  }

  u32 nmask = size - 1;

  ForEachIndex(i, cap) {
    if (!is_stale(meta[i])) {
      continue;
    }

    Bucket<K, V> bi = _buckets[i];

    u32 hash  = hash_key(context, bi.key);
    u32 start = hash & nmask;

    u32 idx = index_not_found;
    ForEachIndex(k, max_search_depth) {
      idx = (start + k) & nmask;
      if (is_empty(meta[idx]) || is_stale(meta[idx])) {
        break;
      }
    }

    // assert(idx != Index_not_found);

    if (i == idx) {
      meta[idx] = fingerprint32(hash) | mask_is_occupied;
      continue;
    }

    meta[i] = 0;

    if (is_empty(meta[idx])) {
      meta[idx]     = fingerprint32(hash) | mask_is_occupied;
      _buckets[idx] = bi;
      continue;
    }

    if (is_stale(meta[idx])) {
      meta[idx] = fingerprint32(hash) | mask_is_occupied;
      swap(bi, _buckets[idx]);
    }

    while (true) {
      u32 hash  = hash_key(context, bi.key);
      u32 start = hash & nmask;

      ForEachIndex(k, max_search_depth) {
        u32 idx = (start + k) & nmask;

        if (is_empty(meta[idx])) {
          meta[idx]     = fingerprint32(hash) | mask_is_occupied;
          _buckets[idx] = bi;
          goto next;
        }

        if (is_stale(meta[idx])) {
          meta[idx] = fingerprint32(hash) | mask_is_occupied;
          swap(bi, _buckets[idx]);
          break;
        }
      }
    }

  next:
    continue;
  }

  this->mask    = nmask;
  this->meta    = meta;
  this->buckets = _buckets;
}

template<typename K, typename V, KeyCmpFn<K> cmp_key, HashFn<K> hash_key>
void HashMap<K, V, cmp_key, hash_key>::insert(K key, V val) {
  b32 was_occupied;
  auto bucket = insert_key_and_get_bucket(key, &was_occupied);
  bucket->val = val;
}

template<typename K, typename V, KeyCmpFn<K> cmp_key, HashFn<K> hash_key>
Bucket<K, V> *
HashMap<K, V, cmp_key, hash_key>::insert_key_and_get_bucket(K key, b32 *was_occupied) {
  u32 cap = mask + 1;
  if (len > cap * max_load_factor) {
    u32 size = max(min_map_size, cap * 2);
    grow_and_rehash(size);
  }

  u32 hash = hash_key(context, key);
  u32 idx  = get_insert_index(hash, key);

  while (idx == index_not_found) {
    u32 cap  = mask + 1;
    u32 size = max(min_map_size, cap * 2);
    grow_and_rehash(size);
    idx = get_insert_index(hash, key);
  }

  b32 _is_occupied = is_occupied(meta[idx]);

  if (!_is_occupied) {
    len              += 1;
    meta[idx]         = fingerprint32(hash) | mask_is_occupied;
    buckets[idx].key  = key;
  }

  *was_occupied = _is_occupied;

  return &buckets[idx];
}

template<typename K, typename V, KeyCmpFn<K> cmp_key, HashFn<K> hash_key>
Bucket<K, V> *HashMap<K, V, cmp_key, hash_key>::remove_key(K key) {
  u32 idx = get_occupied_index(key);
  if (idx == index_not_found) {
    return nullptr;
  }

  len -= 1;

  meta[idx] = mask_is_tombstone;

  return &buckets[idx];
}

template<typename K, typename V, KeyCmpFn<K> cmp_key, HashFn<K> hash_key>
Bucket<K, V> *HashMap<K, V, cmp_key, hash_key>::get_bucket(K key) {
  u32 idx = get_occupied_index(key);
  if (idx == index_not_found) {
    return nullptr;
  }

  return &buckets[idx];
}

template<typename K, typename V, KeyCmpFn<K> cmp_key, HashFn<K> hash_key>
u32 HashMap<K, V, cmp_key, hash_key>::get_occupied_index(K key) {
  u32 hash        = hash_key(context, key);
  u32 fingerprint = fingerprint32(hash);
  u32 start       = hash & mask;

  ForEachIndex(j, max_search_depth) {
    u32 idx = (start + j) & mask;

    if (is_empty(meta[idx])) {
      return index_not_found;
    }

    if (is_occupied(meta[idx]) && fingerprint32(meta[idx]) == fingerprint) {
      if (cmp_key(context, key, buckets[idx].key)) {
        return idx;
      }
    }
  }

  return index_not_found;
}

template<typename K, typename V, KeyCmpFn<K> cmp_key, HashFn<K> hash_key>
u32 HashMap<K, V, cmp_key, hash_key>::get_insert_index(u32 hash, K key) {
  u32 fingerprint = fingerprint32(hash);
  u32 start       = hash & mask;

  u32 tombstone_idx = index_not_found;

  ForEachIndex(j, max_search_depth) {
    u32 idx = (start + j) & mask;

    if (is_empty(meta[idx])) {
      return min(idx, tombstone_idx);
    }

    if (has_tombstone(meta[idx]) && tombstone_idx == index_not_found) {
      tombstone_idx = idx;
    }

    if (is_occupied(meta[idx]) && fingerprint32(meta[idx]) == fingerprint) {
      if (cmp_key(context, key, buckets[idx].key)) {
        return idx;
      }
    }
  }

  return tombstone_idx;
}
