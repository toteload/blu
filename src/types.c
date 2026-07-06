#include "types.h"

#define XXH_INLINE_ALL
#include "xxhash.h"

internal b32 cmp_type(void *context, Type *a, Type *b);
internal u32 hash_type(void *context, Type *x);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#define HASHMAP_NAME            UniqueTypeMap
#define HASHMAP_KEY_TYPE        TypePtr
#define HASHMAP_VALUE_TYPE      TypeIndex
#define HASHMAP_FUNCTION_PREFIX map
#define HASHMAP_HASH_FN         hash_type
#define HASHMAP_KEY_COMPARE_FN  cmp_type
#define HASHMAP_LINKAGE         internal
#define HASHMAP_OUTPUT_DEFINITIONS
#include "hashmap.h"
#pragma clang diagnostic pop

#define SEGMENTLIST_NAME            TypeList
#define SEGMENTLIST_TYPE            TypePtr
#define SEGMENTLIST_FUNCTION_PREFIX list
#define SEGMENTLIST_MIN_SIZE_LOG2   TypeList_min_size_log2 
#define SEGMENTLIST_SEGMENT_COUNT   TypeList_segment_count
#define SEGMENTLIST_LINKAGE         internal
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

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

    for (u32 i = 0; i < a->data.function.param_count; i++) {
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
    void *_p = arena_push(arena, sizeof(y), 1);                                                    \
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
    for (u32 i = 0; i < x->data.function.param_count; i++) { Push_data(x->function_param_types[i]); }
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

void types_init(TypeInterner *types, TypeInternerOptions *options) {
  types->arena = options->arena;
  types->arena_scratch = options->arena_scratch;
  zero_struct(TypeList, &types->list);
  map_init(&types->map, &(HashMapOptions){
    .allocator    = options->map_allocator,
    .initial_size = 32,
    .context      = options->arena_scratch,
  });
}

void types_deinit(TypeInterner *types) {
  map_deinit(&types->map);
  zero_struct(TypeInterner, types);
}

TypeIndex types_add(TypeInterner *types, Type *type) {
  b32 was_occupied;
  UniqueTypeMapBucket *bucket = map_insert_key_and_get_bucket(&types->map, type, &was_occupied);

  if (was_occupied) {
    return bucket->val;
  }

  u32 size = type_intern_byte_size(type);
  Type *intern = arena_push(types->arena, size, Align_of(Type));
  memcpy(intern, type, size);

  u32 idx = types->list.len;

  *bucket = (UniqueTypeMapBucket){ .key = intern, .val = idx, };

  list_append(&types->list, types->arena, intern);

  return idx;
}

Type *types_get(TypeInterner *types, TypeIndex idx) {
  return *list_ptr_at_unchecked(&types->list, idx);
}

u32 type_intern_byte_size(Type *type) {
  switch (type->kind) {
  case Type_integer:
  case Type_boolean:
  case Type_nil:
  case Type_never:
  case Type_type:
  case Type_slice:
  case Type_array:
    return sizeof(Type);
  case Type_function:
    return sizeof(Type) + type->data.function.param_count * sizeof(TypeIndex);
  }

  Unreachable();

  return 0;
}

TypeSizeInfo types_size_info(TypeInterner *types, Type *type) {
  // ASSUME: pointers are 8 bytes.

  switch (type->kind) {
  case Type_boolean:
    return (TypeSizeInfo){ .size = 1, .align = 1, .stride = 1 };
  case Type_nil:
    return (TypeSizeInfo){ .size = 0, .align = 0, .stride = 0 };
  case Type_never:
    return (TypeSizeInfo){ .size = 0, .align = 0, .stride = 0 };
  case Type_type:
    return (TypeSizeInfo){ .size = 0, .align = 0, .stride = 0 };
  case Type_integer: { 
    // ASSUME: the bitwidth of integers is always a multiple of 8.
    u32 size = type->data.integer.bitwidth / 8;
    return (TypeSizeInfo){ .size = size, .align = size, .stride = size };
  }
  case Type_slice: {
    // A slice is stored as an 8-byte pointer and an 8-byte length.
    return (TypeSizeInfo){ .size = 16, .align = 8, .stride = 16 };
  }
  case Type_array: {
    TypeSizeInfo size_info = types_size_info_by_index(types, type->data.array.base_type);
    u32 size = size_info.stride * type->data.array.size;
    return (TypeSizeInfo){ 
      .size   = size,
      .align  = size_info.align, 
      .stride = size,
    };
  }
  case Type_function: {
    // A variable holding a function is stored as an 8-byte pointer to the actual code of the function.
    return (TypeSizeInfo){ .size = 8, .align = 8, .stride = 8 };
  }
  }

  Unreachable();

  return (TypeSizeInfo){0};
}

TypeSizeInfo types_size_info_by_index(TypeInterner *types, TypeIndex idx) {
  Type *type = types_get(types, idx);
  return types_size_info(types, type);
}
