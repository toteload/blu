#include "blu.hh"

#define XXH_INLINE_ALL
#include "xxhash.h"

b32 type_eq(void *context, Type *a, Type *b) {
  if (a->kind != b->kind) {
    return false;
  }

  switch (a->kind) {
  case Type_Void:
  case Type_IntegerConstant:
  case Type_Never:
  case Type_Boolean:
    return true;
  case Type_Function: {
    if (!type_eq(context, a->function.return_type, b->function.return_type)) {
      return false;
    }

    if (a->function.param_count != b->function.param_count) {
      return false;
    }

    ForEachIndex(i, a->function.param_count) {
      if (!type_eq(context, a->function.params[i], b->function.params[i])) {
        return false;
      }
    }

    return true;
  }
  case Type_Integer:
    return a->integer.signedness == b->integer.signedness &&
           a->integer.bitwidth == b->integer.bitwidth;
  }
}

u32 push_type_data(Arena *arena, Type *x) {
#define Push_data(y)                                                                               \
  {                                                                                                \
    void *_p = arena->raw_alloc(sizeof(y), 1);                                                     \
    memcpy(_p, &(y), sizeof(y));                                                                   \
    size += sizeof(y);                                                                             \
  }

  u32 size = 0;

  Push_data(x->kind);

  switch (x->kind) {
  case Type_Void:
  case Type_IntegerConstant:
  case Type_Boolean:
    break;
  case Type_Integer: {
    Push_data(x->integer.signedness);
    Push_data(x->integer.bitwidth);
  } break;
  case Type_Function: {
    push_type_data(arena, x->function.return_type);
    Push_data(x->function.param_count);
    ForEachIndex(i, x->function.param_count) { push_type_data(arena, x->function.params[i]); }
  } break;
  default:
    Unreachable();
  }

  return size;

#undef Push_data
}

u32 type_hash(void *context, Type *x) {
  Arena *arena  = cast<Arena *>(context);
  auto snapshot = arena->take_snapshot();

  void *data = snapshot.at;
  u32 size   = push_type_data(arena, x);
  u32 hash   = XXH32(data, size, 0);

  arena->restore(snapshot);

  return hash;
}

void TypeInterner::init(Arena *arena, Allocator storage_allocator, Allocator map_allocator) {
  storage = storage_allocator;
  map.init(map_allocator, arena);

#define Add_type(Identifier, T)                                                                    \
  {                                                                                                \
    auto _tmp  = T;                                                                                \
    Identifier = add(&_tmp);                                                                       \
  }

  Add_type(_bool, Type::make_bool());
  Add_type(_i32, Type::make_integer(Signed, 32));
  Add_type(_integer_constant, Type::make_integer_constant());
  Add_type(_void, Type::make_void());

#undef Add_type
}

void TypeInterner::deinit() {}

Type *TypeInterner::add(Type *type) {
  b32 was_occupied;
  auto bucket = map.insert_key_and_get_bucket(type, &was_occupied);
  if (was_occupied) {
    return bucket->val;
  }

  // TODO: using the user provided type pointer as the key is wrong as we don't control the memory.
  // Also need to recursively add all the types.

  u32 byte_size = type->byte_size();
  Type *intern  = cast<Type *>(storage.raw_alloc(byte_size, Align_of(Type)));
  memcpy(intern, type, byte_size);

  bucket->key = intern;
  bucket->val = intern;

  return intern;
}
