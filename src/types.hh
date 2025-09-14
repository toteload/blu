enum TypeKind : u8 {
  Type_integer_constant,
  Type_integer,
  Type_boolean,
  Type_function,
  Type_nil,
  Type_never,
  Type_slice,
  Type_distinct,
  Type_sequence,
  Type_type,
};

enum Signedness : u8 {
  Signed,
  Unsigned,
};

struct Type {
  TypeKind kind;

  union {
    struct {
      Signedness signedness;
      u16 bitwidth;
    } integer;
    struct {
      Type *base_type;
    } slice;
    struct {
      Type *return_type;
      u32 param_count;
      Type *params[0];
    } function;
    struct {
      u32 uid;
      Type *base;
    } distinct;
    struct {
      u32 count;
      Type *items[0];
    } sequence;
  };

  b32 is_sized_type() { return kind != Type_nil && kind != Type_never; }

  u32 write_string(Slice<char> out);

  u32 byte_size() {
    switch (kind) {
    case Type_integer:
    case Type_integer_constant:
    case Type_nil:
    case Type_never:
    case Type_slice:
    case Type_distinct:
    case Type_type:
    case Type_boolean:
      return sizeof(*this);
    case Type_function: {
      return sizeof(*this) + function.param_count * sizeof(Type *);
    } break;
    case Type_sequence:
      return sizeof(*this) + sequence.count * sizeof(Type *);
    }
  }

  bool is_integer_or_integer_constant() {
    return kind == Type_integer || kind == Type_integer_constant;
  }

  static Type make_slice(Type *base) {
    Type t;
    t.kind            = Type_slice;
    t.slice.base_type = base;
    return t;
  }
  static Type make_nil() { return {Type_nil, {}}; }
  static Type make_bool() { return {Type_boolean, {}}; }
  static Type make_never() { return {Type_never, {}}; }
  static Type make_type() { return {Type_type, {}}; }
  static Type make_integer_constant() { return {Type_integer_constant, {}}; }
  static Type make_integer(Signedness s, u16 width) {
    return {
      Type_integer,
      {{
        s,
        width,
      }},
    };
  }
};

b32 type_eq(void *context, Type *a, Type *b);
u32 type_hash(void *context, Type *x);

struct TypeKey {
  u32 idx;
};

struct TypeInterner {
  Allocator storage;
  HashMap<Type *, Type *, type_eq, type_hash> map; // TODO this should be a set
  u32 distinct_uid_gen = 0;

  // Often used and always available types
  Type *bool_;
  Type *u8_;
  Type *u16_;
  Type *u32_;
  Type *u64_;
  Type *i8_;
  Type *i16_;
  Type *i32_;
  Type *i64_;
  Type *integer_constant;
  Type *nil;
  Type *never;

  void init(Arena *work_arena, Allocator storage_allocator, Allocator map_allocator);
  void deinit();

  u32 _get_new_distinct_uid() {
    u32 uid           = distinct_uid_gen;
    distinct_uid_gen += 1;
    return uid;
  }

  Type *_intern_type(Type *type);

  Type *add(Type *type);

  // Intern the provided type, but make it distinct as well.
  // Two calls to this function with the same pointer will result in two different types returned.
  Type *add_as_distinct(Type *type);
};
