#include "env.h"

always_inline internal u32 hash_string_index(void *context, StringIndex idx) {
  Unused(context);

  return idx;
}

always_inline internal b32 cmp_string_index(void *context, StringIndex a, StringIndex b) {
  Unused(context);

  return a == b;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#define HASHMAP_NAME            DeclarationMap
#define HASHMAP_FUNCTION_PREFIX map
#define HASHMAP_KEY_TYPE        StringIndex
#define HASHMAP_VALUE_TYPE      Declaration
#define HASHMAP_HASH_FN         hash_string_index
#define HASHMAP_KEY_COMPARE_FN  cmp_string_index
#define HASHMAP_LINKAGE         internal
#define HASHMAP_OUTPUT_DEFINITIONS
#include "hashmap.h"
#pragma clang diagnostic pop

void env_insert(Env *env, StringIndex idx, Declaration decl) {
  b32 was_occupied_ignore;
  DeclarationMapBucket *bucket = map_insert_key_and_get_bucket(&env->map, idx, &was_occupied_ignore);
  bucket->val = decl;
}

b32 env_contains(Env *env, StringIndex idx) {
  Declaration *decl = map_find(&env->map, idx);
  return !is_null(decl);
}

b32 env_lookup(Env *env, StringIndex idx, Declaration *decl) {
  Declaration *p = map_find(&env->map, idx);
  if (is_null(p)) {
    return False;
  }

  *decl = *p;

  return True;
}

void envs_init(EnvAllocator *envs, EnvAllocatorOptions *options) {
  envs->arena = options->arena;
  envs->map_allocator = options->map_allocator;
}

Env *envs_alloc(EnvAllocator *envs, Env *parent) {
  if (is_null(envs->freelist)) {
    usize reserve_amount = 8;
    void *items = arena_push_array(Env, envs->arena, reserve_amount);
    freelist_grow(&envs->freelist, items, sizeof(Env), reserve_amount);
  }

  Env *env = freelist_alloc(&envs->freelist);

  env->parent = parent;
  map_init(&env->map, &(HashMapOptions){
    .allocator = envs->map_allocator,
    .initial_size = 8,
  });

  return env;
}

void envs_dealloc(EnvAllocator *envs, Env *env) {
  map_deinit(&env->map);
  freelist_free(&envs->freelist, env);
}
