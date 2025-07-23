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

  void init_global_env(StringInterner *strings, TypeInterner *types) {
    global_env = pool.alloc();

#define Add_type(Identifier, T)                                                                    \
  {                                                                                                \
    auto _id  = strings->add(Str_make(Identifier));                                            \
    auto _tmp = T;                                                                                 \
    auto _t   = types->add(&_tmp);                                                             \
    global_env->insert(_id, Value::make_type(_t));                                    \
  }

  // clang-format off
  Add_type("i8",  Type::make_integer(Signed,  8));
  Add_type("i16", Type::make_integer(Signed, 16));
  Add_type("i32", Type::make_integer(Signed, 32));
  Add_type("i64", Type::make_integer(Signed, 64));

  Add_type("u8",  Type::make_integer(Unsigned,  8));
  Add_type("u16", Type::make_integer(Unsigned, 16));
  Add_type("u32", Type::make_integer(Unsigned, 32));
  Add_type("u64", Type::make_integer(Unsigned, 64));

  Add_type("bool", Type::make_bool());
  Add_type("void", Type::make_void());
  // clang-format on

#undef Add_type

  {
    auto id = strings->add(Str_make("true"));
    global_env->insert(id, Value::make_builtin(types->bool_));
  }
  {
    auto id = strings->add(Str_make("false"));
    global_env->insert(id, Value::make_builtin(types->bool_));
  }

  }

  void deinit();

  Env *alloc(Env *parent = nullptr) {
    Env *env = pool.alloc();
    if (!parent) {
      parent = global_env;
    }
    env->init(env_allocator, parent);
    return env;
  }

  void dealloc(Env *env) {
    env->deinit();
    pool.dealloc(env);
  }
};
