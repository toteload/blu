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
  case Type_Pointer: {
    return type_eq(context, a->pointer.base_type, b->pointer.base_type);
  }
  case Type_type: {
      return type_eq(context, a->type.base, b->type.base);
    }
  case Type_slice: {
    return type_eq(context, a->slice.base_type, b->slice.base_type);
  }
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
  case Type_distinct: {
    return a->distinct.uid == b->distinct.uid;
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
  case Type_Never:
  case Type_Boolean:
    break;
  case Type_distinct: {
    Push_data(x->distinct.uid);
  } break;
  case Type_Integer: {
    Push_data(x->integer.signedness);
    Push_data(x->integer.bitwidth);
  } break;
  case Type_Pointer: {
    push_type_data(arena, x->pointer.base_type);
  } break;
    case Type_type: {
      push_type_data(arena, x->type.base);
    } break;
  case Type_slice: {
    push_type_data(arena, x->slice.base_type);
  } break;
  case Type_Function: {
    push_type_data(arena, x->function.return_type);
    Push_data(x->function.param_count);
    ForEachIndex(i, x->function.param_count) { push_type_data(arena, x->function.params[i]); }
  } break;
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

  Add_type(bool_, Type::make_bool());
  Add_type(i32_, Type::make_integer(Signed, 32));
  Add_type(integer_constant, Type::make_integer_constant());
  Add_type(void_, Type::make_void());
  Add_type(never, Type::make_never());

#undef Add_type
}

void TypeInterner::deinit() {}

Type *TypeInterner::_intern_type(Type *type) {
  u32 byte_size = type->byte_size();
  Type *intern  = cast<Type *>(storage.raw_alloc(byte_size, Align_of(Type)));
  memcpy(intern, type, byte_size);

  switch (type->kind) {
  case Type_Void:
  case Type_IntegerConstant:
  case Type_Never:
  case Type_Boolean:
  case Type_Integer:
    break;
  case Type_distinct: {
    intern->distinct.base = add(type->distinct.base);
  } break;
    case Type_type: {
      intern->type.base = add(type->type.base);
    } break;
  case Type_Pointer: {
    intern->pointer.base_type = add(type->pointer.base_type);
  } break;
  case Type_slice: {
    intern->slice.base_type = add(type->slice.base_type);
  } break;
  case Type_Function: {
    intern->function.return_type = add(type->function.return_type);
    for (usize i = 0; i < type->function.param_count; i += 1) {
      intern->function.params[i] = add(type->function.params[i]);
    }
  } break;
  }

  return intern;
}

Type *TypeInterner::add(Type *type) {
  b32 was_occupied;
  auto bucket = map.insert_key_and_get_bucket(type, &was_occupied);
  if (was_occupied) {
    return bucket->val;
  }

  Type *intern = _intern_type(type);

  bucket->key = intern;
  bucket->val = intern;

  return intern;
}

Type *TypeInterner::add_as_distinct(Type *type) {
  Type *t = _intern_type(type);

  Type *distinct_type = storage.alloc<Type>();

  distinct_type->kind          = Type_distinct;
  distinct_type->distinct.uid  = _get_new_distinct_uid();
  distinct_type->distinct.base = t;

  map.insert(distinct_type, distinct_type);

  return distinct_type;
}

Type *TypeInterner::add_as_type(Type *type) {
  Type *t = _intern_type(type);

  Type *type_type = storage.alloc<Type>();

  type_type->kind = Type_type;
  type_type->type.base = t;

  map.insert(type_type, type_type);

  return type_type;
}
