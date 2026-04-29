ttld_inline b32 str_key_eq(void *context, StrKey a, StrKey b) { return a.idx == b.idx; }
ttld_inline u32 str_key_hash(void *context, StrKey x) { return x.idx; }

struct Env {
  Env *parent;
  HashMap<StrKey, ValueIndex, str_key_eq, str_key_hash> map;

  void init(Allocator allocator, Env *parent = nullptr) {
    map.init(allocator);
    this->parent = parent;
  }

  void deinit() {
    parent = nullptr;
    map.deinit();
  }

  void insert(StrKey identifier, ValueIndex val) { map.insert(identifier, val); }

  OptionalValueIndex lookup(StrKey identifier) {
    ValueIndex *p = map.get_ptr(identifier);
    if (p) {
      return OptionalValueIndex::from_index(*p);
    }

    if (parent) {
      return parent->lookup(identifier);
    }

    return OptionalValueIndex::none();
  }
};

struct EnvManager {
  ObjectPool<Env> pool;

  // Each Env uses the env_allocator for its own allocations.
  Allocator env_allocator;

  void init(Allocator pool_allocator, Allocator env_allocator) {
    pool.init(pool_allocator);
    this->env_allocator = env_allocator;
  }

  void deinit();

  Env *alloc(Env *parent) {
    Env *env = pool.alloc();
    env->init(env_allocator, parent);
    return env;
  }

  void dealloc(Env *env) {
    env->deinit();
    pool.dealloc(env);
  }

  void _add_type(
    StringInterner *strings,
    TypeInterner *types,
    ValueStore *values,
    Env *env,
    Str identifier,
    Type type
  ) {
    auto key = strings->add(identifier);
    auto ty  = types->add(&type);
    auto val = values->add({.kind = Val_type, .type = types->type.type, .data = {.type = ty}});
    env->insert(key, val);
  }

  Env *create_global_env(StringInterner *strings, TypeInterner *types, ValueStore *values) {
    auto global_env = alloc(nullptr);

#define Add_type(Identifier, Type)                                                                 \
  _add_type(strings, types, values, global_env, Str_make(Identifier), Type)

    // clang-format off
    Add_type( "i8", Type::make_integer(Signed,  8));
    Add_type("i16", Type::make_integer(Signed, 16));
    Add_type("i32", Type::make_integer(Signed, 32));
    Add_type("i64", Type::make_integer(Signed, 64));

    Add_type( "u8", Type::make_integer(Unsigned,  8));
    Add_type("u16", Type::make_integer(Unsigned, 16));
    Add_type("u32", Type::make_integer(Unsigned, 32));
    Add_type("u64", Type::make_integer(Unsigned, 64));

    Add_type("nil",   Type::make_nil());
    Add_type("never", Type::make_never());
    Add_type("type",  Type::make_type());
    Add_type("bool",  Type::make_bool());
    // clang-format on
#undef Add_type

    TypeIndex bool_type;
    {
      auto key   = strings->add(Str_make("bool"));
      auto idx   = global_env->lookup(key);
      Value *val = values->get(idx.as_index());
      bool_type  = val->data.type;
    }

    global_env->insert(
      strings->add(Str_make("true")),
      values->add({
        .kind = Val_true,
        .type = bool_type,
        .data = {},
      })
    );

    global_env->insert(
      strings->add(Str_make("false")),
      values->add({
        .kind = Val_false,
        .type = bool_type,
        .data = {},
      })
    );

    return global_env;
  }
};
