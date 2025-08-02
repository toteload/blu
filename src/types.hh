enum TypeKind : u8 {
  Type_IntegerConstant,
  Type_Integer,
  Type_Boolean,
  Type_Function,
  Type_Void,
  Type_Never,
  Type_Pointer,
  Type_slice,
  Type_distinct,
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
    } pointer;
    struct {
      Type *base_type;
    } slice;
    struct {
      u32 param_count;
      Type *return_type;
      Type *params[0];
    } function;
    struct {
      u32 uid;
      Type *base;
    } distinct;
    struct {
      Type *base;
    } type;
  };

  b32 is_sized_type() { return kind != Type_Void && kind != Type_Never; }

  u32 byte_size() {
    switch (kind) {
    case Type_Integer:
    case Type_IntegerConstant:
    case Type_Void:
    case Type_Never:
    case Type_Pointer:
    case Type_slice:
    case Type_distinct:
    case Type_type:
    case Type_Boolean:
      return sizeof(*this);
    case Type_Function: {
      return sizeof(*this) + function.param_count * sizeof(Type *);
    } break;
    }
  }

  bool is_integer_or_integer_constant() {
    return kind == Type_Integer || kind == Type_IntegerConstant;
  }

  static Type make_slice(Type *base) {
    Type t;
    t.kind            = Type_slice;
    t.slice.base_type = base;
    return t;
  }
  static Type make_pointer(Type *base) {
    Type t;
    t.kind = Type_Pointer;
    t.pointer.base_type = base;
    return t;
  }
  static Type make_void() { return {Type_Void, {}}; }
  static Type make_bool() { return {Type_Boolean, {}}; }
  static Type make_never() { return {Type_Never, {}}; }
  static Type make_integer_constant() { return {Type_IntegerConstant, {}}; }
  static Type make_integer(Signedness s, u16 width) {
    return {
      Type_Integer,
      {{
        s,
        width,
      }},
    };
  }
};

b32 type_eq(void *context, Type *a, Type *b);
u32 type_hash(void *context, Type *x);

struct TypeInterner {
  Allocator storage;
  HashMap<Type *, Type *, type_eq, type_hash> map; // TODO this should be a set
  u32 distinct_uid_gen = 0;

  // Often used and always available types
  Type *bool_;
  Type *i32_;
  Type *integer_constant;
  Type *void_;
  Type *never;

  void init(Arena *arena, Allocator storage_allocator, Allocator map_allocator);
  void deinit();

  u32 _get_new_distinct_uid() {
    u32 uid           = distinct_uid_gen;
    distinct_uid_gen += 1;
    return uid;
  }

  Type *_intern_type(Type *type);

  Type *add(Type *type);
  Type *add_as_distinct(Type *type);
  Type *add_as_type(Type *type);
};
