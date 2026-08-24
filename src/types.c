#include "types.h"
#include "value.h"

#define XXH_INLINE_ALL
#include "xxhash.h"

internal b32 cmp_type(void *context, Type *a, Type *b);
internal u32 hash_type(void *context, Type *x);

internal Type *intern_type(Arena *arena, Type *x) {
  u32 size = type_intern_byte_size(x);
  Type *intern = arena_push(arena, size, Align_of(Type));
  memcpy(intern, x, size);
  return intern;
}

#define INTERNER_NAME            TypeInterner
#define INTERNER_TYPE            TypePtr
#define INTERNER_INDEX_TYPE      TypeIndex
#define INTERNER_FUNCTION_PREFIX types
#define INTERNER_HASH_FN         hash_type
#define INTERNER_COMPARE_FN      cmp_type
#define INTERNER_COPY_FN         intern_type
#define INTERNER_RESERVE_ZERO_INDEX
#define INTERNER_OUTPUT_DEFINITIONS
#include "interner.h"

internal b32 cmp_type(void *context, Type *a, Type *b) {
  Unused(context);

  if (a->kind != b->kind) {
    return False;
  }

  switch (a->kind) {
  case Type_nil:
  case Type_never:
  case Type_bool:
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
      if (a->data.function.param_types[i] != b->data.function.param_types[i]) {
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
  case Type_bool:
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
    for (u32 i = 0; i < x->data.function.param_count; i++) { Push_data(x->data.function.param_types[i]); }
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

  void *data = scratch->at;
  u32 size = push_type_data(scratch, x);
  u32 hash = XXH32(data, size, 0);

  arena_scope_end(scratch, snapshot);

  return hash;
}

u32 type_intern_byte_size(Type *type) {
  switch (Cast(TypeKind, type->kind)) {
  case Type_comptime_int:
  case Type_integer:
  case Type_bool:
  case Type_nil:
  case Type_never:
  case Type_type:
  case Type_slice:
  case Type_pointer:
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
  case Type_bool:
    return (TypeSizeInfo){ .size = 1, .align = 1, .stride = 1 };
  case Type_nil:
    return (TypeSizeInfo){ .size = 0, .align = 0, .stride = 0 };
  case Type_never:
    return (TypeSizeInfo){ .size = 0, .align = 0, .stride = 0 };
  case Type_type:
    return (TypeSizeInfo){ .size = 4, .align = 4, .stride = 4 };
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
    return (TypeSizeInfo){ .size = sizeof(ValueFunc), .align = Align_of(ValueFunc), .stride = sizeof(ValueFunc) };
  }
  }

  Unreachable();

  return (TypeSizeInfo){0};
}

TypeSizeInfo types_size_info_by_index(TypeInterner *types, TypeIndex idx) {
  Type *type = types_get(types, idx);
  return types_size_info(types, type);
}

b32 is_type_coercible_to(TypeInterner *types, TypeIndex to, TypeIndex from) {
  if (to == from) {
    return True;
  }

  Type *type_to = types_get(types, to);
  Type *type_from = types_get(types, from);

  if (type_from->kind == Type_comptime_int && type_to->kind == Type_integer) {
    return True;
  }

  Todo();
}

TypeIndex types_add_pointer(TypeInterner *types, TypeIndex base_type) {
  return types_add(types, &(Type){ .kind = Type_pointer, .data.pointer = { .base_type = base_type } });
}

b32 check_can_type_add(Type *t) {
  if (t->kind == Type_comptime_int || t->kind == Type_integer) {
    return True;
  }

  return False;
}
