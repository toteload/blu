ttld_inline b32 str_key_eq(void *context, StrKey a, StrKey b) { return a.idx == b.idx; }
ttld_inline u32 str_key_hash(void *context, StrKey x) { return x.idx; }

struct Env {
  Env *parent;
  HashMap<StrKey, Value, str_key_eq, str_key_hash> map;

  void init(Allocator allocator, Env *parent = nullptr) {
    map.init(allocator);
    this->parent = parent;
  }

  void deinit() {
    parent = nullptr;
    map.deinit();
  }

  void insert(StrKey identifier, Value val) { map.insert(identifier, val); }

  Value *lookup(StrKey identifier) {
    Value *p = map.get_ptr(identifier);
    if (p) {
      return p;
    }

    if (parent) {
      return parent->lookup(identifier);
    }

    return nullptr;
  }
};

struct EnvManager {
  ObjectPool<Env> pool;
  Allocator env_allocator;
  Env *global_env = nullptr;

  void init(Allocator pool_allocator, Allocator env_allocator) {
    this->env_allocator = env_allocator;

    pool.init(pool_allocator);
  }

#define Add_type(Identifier, T)                                                                    \
  {                                                                                                \
    auto _id  = strings->add(Str_make(Identifier));                                            \
    auto _tmp = T;                                                                                 \
    auto _t   = types->add(&_tmp);                                                             \
    global_env->insert(_id, Value::make_type(_t));                                    \
  }

  void init_global_env(StringInterner *strings, TypeInterner *types) {
    global_env = alloc(nullptr);

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
    // clang-format on

    Type *bool_type;
    {
      auto tmp_bool_type = Type::make_bool();
      bool_type = types->add(&tmp_bool_type);
      auto key = strings->add(Str_make("bool"));
      global_env->insert(key, Value::make_type(bool_type));
    }

    global_env->insert(strings->add(Str_make("true")), { Value_true, bool_type, {}, });
    global_env->insert(strings->add(Str_make("false")), { Value_false, bool_type, {}, });
  }

#undef Add_type

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
};
