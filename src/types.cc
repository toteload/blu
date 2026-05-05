#include "blu.hh"

#define XXH_INLINE_ALL
#include "xxhash.h"

// Should this Slice<char> be a Str?
// Should Str be a subtype of Slice?
u32 type_to_string(TypeInterner *types, TypeIndex idx, char *buf, u32 buf_size) {
  u32 buf_size_start = buf_size;

  Type *type = types->get(idx);

#define Update(Code)                                                                               \
  do {                                                                                             \
    u32 _offset = (Code);                                                                          \
    if (_offset >= buf_size) {                                                                     \
      buf_size  = 0;                                                                               \
      buf      += buf_size;                                                                        \
    } else {                                                                                       \
      buf_size -= _offset;                                                                         \
      buf      += _offset;                                                                         \
    }                                                                                              \
  } while (false)

  switch (type->kind) {
  case Type_integer:
    Update(snprintf(
      buf,
      buf_size,
      "%s%d",
      type->integer.signedness == Signed ? "i" : "u",
      type->integer.bitwidth
    ));
    break;
  case Type_array:
    Update(snprintf(buf, buf_size, "[%llu]", type->array.size));
    Update(type_to_string(types, type->array.base_type, buf, buf_size));
    break;
  case Type_sequence:
    Update(snprintf(buf, buf_size, ".{ "));
    for (u32 i = 0; i < type->sequence.count; i++) {
      Update(type_to_string(types, type->sequence.item_types[i], buf, buf_size));
      Update(snprintf(buf, buf_size, ", "));
    }
    Update(snprintf(buf, buf_size, "}"));
    break;
  case Type_literal_int:
    Update(snprintf(buf, buf_size, "literal-int"));
    break;
  case Type_boolean:
    Update(snprintf(buf, buf_size, "bool"));
    break;
  case Type_type:
    Update(snprintf(buf, buf_size, "type"));
    break;
  case Type_nil:
    Update(snprintf(buf, buf_size, "nil"));
    break;
  case Type_never:
    Update(snprintf(buf, buf_size, "never"));
    break;
  case Type_slice: {
    Update(snprintf(buf, buf_size, "[]"));
    Update(type_to_string(types, type->slice.base_type, buf, buf_size));
  } break;
  case Type_distinct: {
    Update(snprintf(buf, buf_size, "distinct(%d) ", type->distinct.uid));
    Update(type_to_string(types, type->distinct.base_type, buf, buf_size));
  } break;
  case Type_function: {
    Update(snprintf(buf, buf_size, "("));
    if (type->function.param_count > 0) {
      Update(type_to_string(types, type->function.param_types[0], buf, buf_size));
    }
    for (u32 i = 1; i < type->function.param_count; i += 1) {
      Update(type_to_string(types, type->function.param_types[i], buf, buf_size));
    }
    Update(snprintf(buf, buf_size, "): "));
    Update(type_to_string(types, type->function.return_type, buf, buf_size));
  } break;
  case Type_literal_function: {
    Update(snprintf(buf, buf_size, "|%d|: ", type->literal_function.param_count));
    Update(type_to_string(types, type->literal_function.return_type, buf, buf_size));
  } break;
  }

#undef Update

  return buf_size_start - buf_size;
}

b32 type_eq(void *context, Type *a, Type *b) {
  if (a->kind != b->kind) {
    return false;
  }

  switch (a->kind) {
  case Type_nil:
  case Type_literal_int:
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
  case Type_literal_function: {
    return a->literal_function.param_count == b->literal_function.param_count &&
           a->literal_function.return_type == b->literal_function.return_type;
  }
  case Type_array: {
    return a->array.size == b->array.size && a->array.base_type == b->array.base_type;
  }
  }

  return false;
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
  case Type_literal_int:
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
  case Type_literal_function: {
    Push_data(x->function.param_count);
    Push_data(x->literal_function.return_type);
  } break;
  case Type_array: {
    Push_data(x->array.size);
    Push_data(x->array.base_type);
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
  Add_type(type.u8_,  ((Type){ .kind = Type_integer, .integer = { .signedness = Unsigned, .bitwidth =  8 }}));
  Add_type(type.u16_, ((Type){ .kind = Type_integer, .integer = { .signedness = Unsigned, .bitwidth = 16 }}));
  Add_type(type.u32_, ((Type){ .kind = Type_integer, .integer = { .signedness = Unsigned, .bitwidth = 32 }}));
  Add_type(type.u64_, ((Type){ .kind = Type_integer, .integer = { .signedness = Unsigned, .bitwidth = 64 }}));

  Add_type(type.i8_,  ((Type){ .kind = Type_integer, .integer = { .signedness = Signed, .bitwidth =  8 }}));
  Add_type(type.i16_, ((Type){ .kind = Type_integer, .integer = { .signedness = Signed, .bitwidth = 16 }}));
  Add_type(type.i32_, ((Type){ .kind = Type_integer, .integer = { .signedness = Signed, .bitwidth = 32 }}));
  Add_type(type.i64_, ((Type){ .kind = Type_integer, .integer = { .signedness = Signed, .bitwidth = 64 }}));

  // TODO this actually depends on the platform. not every platform is 64 bit :)
  Add_type(type.uint, ((Type){ .kind = Type_integer, .integer = { .signedness = Unsigned, .bitwidth = 64 }}));

  Add_type(type.bool_, ((Type){ .kind = Type_boolean, }));
  Add_type(type.nil,   ((Type){ .kind = Type_nil, }));
  Add_type(type.never, ((Type){ .kind = Type_never, }));

  Add_type(type.literal_int, ((Type){ .kind = Type_literal_int, }));

  Add_type(type.slice_u8, ((Type){.kind = Type_slice, .slice = { .base_type = type.u8_ }}));
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

bool TypeInterner::is_coercible_to(TypeIndex src, TypeIndex dst) {
  if (src == dst) {
    return true;
  }

  auto a = get(src);
  auto b = get(dst);

  if (a->kind == Type_literal_int && b->kind == Type_integer) {
    // TODO: make sure value fits in destination integer type
    return true;
  }

  if (a->kind == Type_sequence) {
    TypeIndex base_type;
    if (b->kind == Type_array) {
      base_type = b->array.base_type;
    } else if (b->kind == Type_slice) {
      base_type = b->slice.base_type;
    } else {
      return false;
    }

    if (b->kind == Type_array && a->sequence.count != b->array.size) {
      return false;
    }

    for (u32 i = 0; i < a->sequence.count; i++) {
      if (!is_coercible_to(a->sequence.item_types[i], base_type)) {
        return false;
      }
    }

    return true;
  }

  if (a->kind == Type_array && b->kind == Type_slice) {
    return a->array.base_type == b->slice.base_type;
  }

  if (a->kind == Type_literal_function && b->kind == Type_function) {
    if (a->literal_function.param_count == b->function.param_count) {
      return is_coercible_to(a->literal_function.return_type, b->function.return_type);
    }
  }

  return false;
}
