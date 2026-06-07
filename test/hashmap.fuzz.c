#include <stdio.h>
#include <time.h>

#include "toteload.h"
#include "rng.h"

//#define ENABLE_DEBUG_PRINT 1

#include "khash.h"
KHASH_MAP_INIT_INT(k32, i32)

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

enum HashMap_Op {
  Op_insert,

  Op_remove_existing_key,
  Op_try_remove_with_random_key,

  Op_get_existing_bucket,
  Op_try_get_random_bucket,

  Op_ops_count,

  Op_end_of_test,
};

void assert_has_all_baseline_items(khash_t(k32) *baseline, IntMap *map) {
  Assert(map->item_count == kh_size(baseline));

  for (khint_t k = kh_begin(baseline); k != kh_end(baseline); k++) {
    if (!kh_exist(baseline, k)) {
      continue;
    }

    i32 key = kh_key(baseline, k);
    i32 *val = map_find(map, key);

    Assert(!Is_null(val));
    Assert(*val == kh_val(baseline, k));
  }
}

i32 random_key(PRng *rng) {
  return PRng_i32(rng) % 1000;
}

i32 random_value(PRng *rng) {
  return PRng_i32(rng) % 1000;
}

enum StrategyKind {
  Strategy_uniform_random,
  Strategy_many_insert_many_remove,
};

typedef struct Strategy {
  u32 kind;
  union {
    struct {
      u32 count;
    } uniform;
    struct {
      u32 insert_count;
      u32 remove_count;
    } many_insert_many_remove;
  };
} Strategy;

u32 next_op(Strategy *s, PRng *rng) {
  switch (s->kind) {
  case Strategy_many_insert_many_remove: {
    while (s->many_insert_many_remove.insert_count) {
      s->many_insert_many_remove.insert_count -= 1;
      return Op_insert;
    }


    while (s->many_insert_many_remove.remove_count) {
      s->many_insert_many_remove.remove_count -= 1;
      return Op_try_remove_with_random_key;
    }

    return Op_end_of_test;
  } break;
  case Strategy_uniform_random: {
    while (s->uniform.count) {
      s->uniform.count -= 1;
      return PRng_u32(rng) % Op_ops_count;
    }

    return Op_end_of_test;
  } break;
  }

  Unreachable();
}

int main() {
  Allocator allocator = { .fn = std_alloc, };
  IntMap map = {};

  u64 run_count = 1000;
  u64 seed = time(NULL);

  PRng rng;
  PRng_seed(&rng, seed);

  Strategy strat = {
#if 0
    .kind = Strategy_many_insert_many_remove,
    .many_insert_many_remove = {
      .insert_count = 10000,
      .remove_count = 1000,
    },
#else
    .kind = Strategy_uniform_random,
    .uniform = {
      .count = 20000,
    },
#endif
  };

  for EachIndex(i, run_count) {
    printf("--- Run %06llu ---\n", i);
    printf("rng { %#018llx, %#018llx, %#018llx, %#018llx, }\n", rng.s[0], rng.s[1], rng.s[2], rng.s[3]);

    map_init(&map, &(HashMapOptions){ .allocator = allocator, .initial_size = 8, });

    khash_t(k32) *baseline = kh_init(k32);

    Strategy s = strat;

    while (True) {
      u32 op = next_op(&s, &rng);

      if (op == Op_end_of_test) {
        break;
      }

      switch (op) {
      case Op_insert: {
        i32 key = random_key(&rng);
        i32 val = random_value(&rng);

        i32 absent;
        khint_t k = kh_put(k32, baseline, key, &absent);
        kh_val(baseline, k) = val;

        map_insert(&map, key, val);

      #ifdef ENABLE_DEBUG_PRINT
        printf("insert (%d, %d)\n", key, val);
      #endif
      } break;
      case Op_try_remove_with_random_key: {
        i32 key = random_key(&rng);

        khint_t k = kh_get(k32, baseline, key);
        kh_del(k32, baseline, k);

        map_remove(&map, key);

      #ifdef ENABLE_DEBUG_PRINT
        printf("try remove for random key %d\n", key);
      #endif
      } break;
      case Op_remove_existing_key: {
        u32 size = kh_size(baseline);
        
        for (khint_t k = kh_begin(baseline); k != kh_end(baseline); k++) {
          if (!kh_exist(baseline, k)) {
            continue;
          }

          u32 roll = PRng_u32(&rng) % size;
          if (roll != 0) {
            continue;
          }

          i32 key = kh_key(baseline, k);

          #ifdef ENABLE_DEBUG_PRINT
          printf("remove for existing key %d\n", key);
          #endif

          kh_del(k32, baseline, k);
          map_remove(&map, key);

          size--;
        }
      } break;
      case Op_try_get_random_bucket: {
        i32 key = random_key(&rng);

        i32 *val = map_find(&map, key);
        khint_t k = kh_get(k32, baseline, key);

        #ifdef ENABLE_DEBUG_PRINT
        printf("try get bucket for random key %d\n", key);
        #endif

        if (Is_null(val)) {
          Assert(k == kh_end(baseline));
        } else {
          Assert(*val == kh_val(baseline, k));
        }
      } break;
      case Op_get_existing_bucket: {
        u32 size = kh_size(baseline);
        
        for (khint_t k = kh_begin(baseline); k != kh_end(baseline); k++) {
          if (!kh_exist(baseline, k)) {
            continue;
          }

          u32 roll = PRng_u32(&rng) % size;
          if (roll != 0) {
            continue;
          }

          size--;

          i32 key = kh_key(baseline, k);

          #ifdef ENABLE_DEBUG_PRINT
          printf("get bucket for existing key %d\n", key);
          #endif

          i32 *val = map_find(&map, key);
          Assert(!Is_null(val));
          Assert(*val == kh_val(baseline, k));
        }
      } break;
      }

      assert_has_all_baseline_items(baseline, &map);
    }

    u32 size = map.item_count;
    u32 baseline_size = kh_size(baseline);

    Assert(size == baseline_size);
        
    for (khint_t k = kh_begin(baseline); k != kh_end(baseline); k++) {
      if (!kh_exist(baseline, k)) {
        continue;
      }

      u32 roll = PRng_u32(&rng) % size;
      if (roll != 0) {
        continue;
      }

      size--;

      i32 key = kh_key(baseline, k);

      i32 *val = map_find(&map, key);
      Assert(!Is_null(val));
      Assert(*val == kh_val(baseline, k));
    }

    printf("Ended with hashmap containing %d items\n", map.item_count);

    kh_destroy(k32, baseline);

    map_deinit(&map);
  }

  printf("ok\n");

  return 0;
}
