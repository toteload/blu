#include "toteload.hh"

struct TypeInterner;

enum TypeKind : u8 {
  Type_literal_int,
  Type_literal_function,
  Type_integer,
  Type_boolean,
  Type_function,
  Type_nil,
  Type_never,
  Type_slice,
  Type_array,
  Type_distinct,
  Type_sequence,
  Type_type,
};

enum Signedness : u8 {
  Signed,
  Unsigned,
};

struct TypeIndexTag {};

using TypeIndex         = Index<u32, TypeIndexTag>;
using OptionalTypeIndex = OptionalIndex<u32, TypeIndexTag>;

struct TypeSizeInfo {
  u32 size;
  u32 stride;
  u32 align;

  template<typename T> static TypeSizeInfo of_type() {
    return {
      .size   = sizeof(T),
      .stride = sizeof(T),
      .align  = Align_of(T),
    };
  }
};

struct Type {
  TypeKind kind;

  union {
    struct {
      Signedness signedness;
      u16 bitwidth;
    } integer;
    struct {
      TypeIndex base_type;
    } slice;
    struct {
      TypeIndex base_type;
      i64 size;
    } array;
    struct {
      TypeIndex return_type;
      u32 param_count;
    } literal_function;
    struct {
      TypeIndex return_type;
      u32 param_count;
      TypeIndex param_types[0];
    } function;
    struct {
      u32 uid;
      TypeIndex base_type;
    } distinct;
    struct {
      u32 count;
      TypeIndex item_types[0];
    } sequence;
  };

  TypeSizeInfo size_info() const {
    switch (kind) {
    case Type_integer:
      return (TypeSizeInfo){
        .size   = integer.bitwidth / cast<u32>(8),
        .stride = integer.bitwidth / cast<u32>(8),
        .align  = integer.bitwidth / cast<u32>(8),
      };
    case Type_literal_int:
    case Type_literal_function:
    case Type_nil:
    case Type_never:
    case Type_slice:
    case Type_array:
    case Type_distinct:
    case Type_type:
    case Type_boolean:
    case Type_function:
    case Type_sequence:
      Todo();
      break;
    }
  }

  b32 is_sized_type() { return kind != Type_nil && kind != Type_never; }

  u32 byte_size() {
    switch (kind) {
    case Type_integer:
    case Type_literal_int:
    case Type_literal_function:
    case Type_nil:
    case Type_never:
    case Type_slice:
    case Type_array:
    case Type_distinct:
    case Type_type:
    case Type_boolean:
      return sizeof(*this);
    case Type_function: {
      return sizeof(*this) + function.param_count * sizeof(TypeIndex);
    } break;
    case Type_sequence:
      return sizeof(*this) + sequence.count * sizeof(TypeIndex);
    default:
      Unreachable();
      return 0;
    }
  }

  bool is_integer_or_literal_int() { return kind == Type_integer || kind == Type_literal_int; }
};

ttld_inline Type *alloc_type_function(Arena *arena, u32 param_count) {
  return cast<Type *>(
    arena->raw_alloc(sizeof(Type) + sizeof(TypeIndex) * param_count, Align_of(Type))
  );
}

ttld_inline Type *alloc_type_sequence(Arena *arena, u32 count) {
  return cast<Type *>(arena->raw_alloc(sizeof(Type) + sizeof(TypeIndex) * count, Align_of(Type)));
}

b32 type_eq(void *context, Type *a, Type *b);
u32 type_hash(void *context, Type *x);

struct TypeInterner {
  Allocator storage;
  HashMap<Type *, TypeIndex, type_eq, type_hash> map;
  Vector<Type *> list;
  u32 distinct_uid_gen = 1;

  // Often used and always available types

  struct {
    TypeIndex bool_;

    TypeIndex u8_;
    TypeIndex u16_;
    TypeIndex u32_;
    TypeIndex u64_;

    TypeIndex i8_;
    TypeIndex i16_;
    TypeIndex i32_;
    TypeIndex i64_;

    TypeIndex uint;

    TypeIndex literal_int;
    TypeIndex literal_string;

    TypeIndex nil;
    TypeIndex never;

    TypeIndex type;

    TypeIndex slice_u8;
  } type;

  void init(
    Arena *work_arena,
    Allocator storage_allocator,
    Allocator map_allocator,
    Allocator list_allocator
  );
  void deinit();

  u32 _get_new_distinct_uid() {
    u32 uid           = distinct_uid_gen;
    distinct_uid_gen += 1;
    return uid;
  }

  TypeIndex _intern_type(Type *type);

  TypeIndex add(Type *type);

  // Intern the provided type, but make it distinct as well.
  // Two calls to this function with the same pointer will result in two different types returned.
  TypeIndex add_as_distinct(Type *type);

  Type *get(TypeIndex idx) { return list[idx.idx]; }

  bool is_coercible_to(TypeIndex src, TypeIndex dst);
};

u32 type_to_string(TypeInterner *types, TypeIndex type, char *buf, u32 buf_size);
