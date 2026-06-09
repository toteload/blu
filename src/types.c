#include "types.h"

#define XXH_INLINE_ALL
#include "xxhash.h"

internal b32 cmp_type(void *context, Type *a, Type *b) {
  if (a->kind != b->kind) {
    return False;
  }

  switch (a->kind) {
  case Type_nil:
  case Type_never:
  case Type_boolean:
  case Type_type:
    return True;
  case Type_slice:
    return a->data.slice.base_type == b->data.slice.base_type;
  case Type_function: {
    if (a->data.function.return_type != b->data.function.return_type) {
      return False;
    }

    if (a->data.function.param_count != b->data.function.param_count) {
      return False;
    }

    for EachIndex(i, a->data.function.param_count) {
      if (a->function_param_types[i] != b->function_param_types[i]) {
        return False;
      }
    }

    return True;
  }
  case Type_integer:
    return a->data.integer.signedness == b->data.integer.signedness &&
           a->data.integer.bitwidth == b->data.integer.bitwidth;
  case Type_array: {
    return a->data.array.size == b->data.array.size && a->data.array.base_type == b->data.array.base_type;
  }
  }

  return False;
}

internal u32 push_type_data(Arena *arena, Type *x) {
#define Push_data(y)                                                                               \
  {                                                                                                \
    void *_p = arena_push(arena, sizeof(y), 1);                                                     \
    memcpy(_p, &(y), sizeof(y));                                                                   \
    size += sizeof(y);                                                                             \
  }

  u32 size = 0;

  Push_data(x->kind);

  switch (x->kind) {
  case Type_nil:
  case Type_never:
  case Type_boolean:
  case Type_type:
    break;
  case Type_integer: {
    Push_data(x->data.integer.signedness);
    Push_data(x->data.integer.bitwidth);
  } break;
  case Type_slice: {
    Push_data(x->data.slice.base_type);
  } break;
  case Type_function: {
    Push_data(x->data.function.return_type);
    Push_data(x->data.function.param_count);
    for EachIndex(i, x->data.function.param_count) { Push_data(x->function_param_types[i]); }
  } break;
  case Type_array: {
    Push_data(x->data.array.size);
    Push_data(x->data.array.base_type);
  } break;
  }

  return size;

#undef Push_data
}
internal u32 hash_type(void *context, Type *x) {
  Arena *scratch  = context;
  ArenaSnapshot snapshot = arena_scope_begin(scratch);

  void *data = scratch->base;
  u32 size = push_type_data(scratch, x);
  u32 hash = XXH32(data, size, 0);

  arena_scope_end(scratch, snapshot);

  return hash;
}

#define HASHMAP_NAME            UniqueTypeMap
#define HASHMAP_KEY_TYPE        TypePtr
#define HASHMAP_VALUE_TYPE      TypeIndex
#define HASHMAP_FUNCTION_PREFIX map
#define HASHMAP_HASH_FN         hash_type
#define HASHMAP_KEY_COMPARE_FN  cmp_type
#define HASHMAP_OUTPUT_DEFINITIONS
#include "hashmap.h"

void types_init(TypeInterner *types, TypeInternerOptions *options) {
  arena_init(&types->arena, &(ArenaOptions){
    .reserve_size = MiB(1),
    .initial_commit_size = KiB(16),
  });
  zero_struct(TypeList, &types->list);
  map_init(&types->map, &(HashMapOptions){
    .allocator    = options->map_allocator,
    .initial_size = 32,
    .context      = options->scratch,
  });
}

void types_deinit(TypeInterner *types);
TypeIndex  types_add(TypeInterner *types, Type *type);
Type      *types_get(TypeInterner *types, TypeIndex idx);


