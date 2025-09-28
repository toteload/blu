#include "blu.hh"

#define XXH_INLINE_ALL
#include "xxhash.h"

b32 type_eq(void *context, Type *a, Type *b) {
  if (a->kind != b->kind) {
    return false;
  }

  switch (a->kind) {
  case Type_nil:
  case Type_integer_constant:
  case Type_never:
  case Type_boolean:
  case Type_type:
    return true;
  case Type_slice:
    return a->slice.base_type == b->slice.base_type;
  case Type_function: {
    if (a->function.return_type != b->function.return_type) {
      return false;
    }

    if (a->function.param_count != b->function.param_count) {
      return false;
    }

    ForEachIndex(i, a->function.param_count) {
      if (a->function.param_types[i] != b->function.param_types[i]) {
        return false;
      }
    }

    return true;
  }
  case Type_sequence: {
    if (a->sequence.count != b->sequence.count) {
      return false;
    }

    ForEachIndex(i, a->sequence.count) {
      if (a->sequence.item_types[i] != b->sequence.item_types[i]) {
        return false;
      }
    }

    return true;
  }
  case Type_distinct: {
    return a->distinct.uid == b->distinct.uid;
  }
  case Type_integer:
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
  case Type_nil:
  case Type_integer_constant:
  case Type_never:
  case Type_boolean:
  case Type_type:
    break;
  case Type_distinct: {
    Push_data(x->distinct.uid);
  } break;
  case Type_integer: {
    Push_data(x->integer.signedness);
    Push_data(x->integer.bitwidth);
  } break;
  case Type_slice: {
    Push_data(x->slice.base_type);
  } break;
  case Type_function: {
    Push_data(x->function.return_type);
    Push_data(x->function.param_count);
    ForEachIndex(i, x->function.param_count) { Push_data(x->function.param_types[i]); }
  } break;
  case Type_sequence: {
    Push_data(x->sequence.count);
    ForEachIndex(i, x->sequence.count) { Push_data(x->sequence.item_types[i]); }
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

void TypeInterner::init(
  Arena *work_arena, Allocator storage_allocator, Allocator map_allocator, Allocator list_allocator
) {
  storage = storage_allocator;
  map.init(map_allocator, work_arena);
  list.init(list_allocator);

#define Add_type(Identifier, T)                                                                    \
  {                                                                                                \
    auto _tmp  = T;                                                                                \
    Identifier = add(&_tmp);                                                                       \
  }

  // clang-format off
  Add_type(u8_,  Type::make_integer(Unsigned, 8));
  Add_type(u16_, Type::make_integer(Unsigned, 16));
  Add_type(u32_, Type::make_integer(Unsigned, 32));
  Add_type(u64_, Type::make_integer(Unsigned, 64));

  Add_type(i8_,  Type::make_integer(Signed,  8));
  Add_type(i16_, Type::make_integer(Signed, 16));
  Add_type(i32_, Type::make_integer(Signed, 32));
  Add_type(i64_, Type::make_integer(Signed, 64));

  Add_type(bool_, Type::make_bool());
  Add_type(nil,   Type::make_nil());
  Add_type(never, Type::make_never());

  Add_type(integer_constant, Type::make_integer_constant());
  // clang-format on

#undef Add_type
}

void TypeInterner::deinit() {}

TypeIndex TypeInterner::add(Type *type) {
  b32 was_occupied;
  auto bucket = map.insert_key_and_get_bucket(type, &was_occupied);
  if (was_occupied) {
    return bucket->val;
  }

  u32 byte_size = type->byte_size();
  Type *intern  = cast<Type *>(storage.raw_alloc(byte_size, Align_of(Type)));
  memcpy(intern, type, byte_size);

  TypeIndex index = {cast<u32>(list.len())};

  list.push(intern);

  bucket->key = intern;
  bucket->val = index;

  return index;
}

TypeIndex TypeInterner::add_as_distinct(Type *type) {
  Type *distinct_type = storage.alloc<Type>();

  distinct_type->kind               = Type_distinct;
  distinct_type->distinct.uid       = _get_new_distinct_uid();
  distinct_type->distinct.base_type = add(type);

  return add(distinct_type);
}
