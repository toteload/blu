#pragma once

ttld_inline b32 str_key_eq(void *context, StrKey a, StrKey b) { return a.idx == b.idx; }
ttld_inline u32 str_key_hash(void *context, StrKey x) { return x.idx; }

template<typename T> struct Env {
  Env                                         *parent;
  HashMap<StrKey, T, str_key_eq, str_key_hash> map;

  void init(Allocator allocator, Env *parent = nullptr) {
    map.init(allocator);
    this->parent = parent;
  }

  void deinit() {
    parent = nullptr;
    map.deinit();
  }

  void insert(StrKey identifier, T val) { map.insert(identifier, val); }
  bool lookup(StrKey identifier, T *out) {
    T *p = map.get_ptr(identifier);
    if (p) {
      *out = *p;
      return true;
    }

    if (parent) {
      return parent->lookup(identifier, out);
    }

    return false;
  }
};

template<typename T> struct EnvManager {
  ObjectPool<Env<T>> pool;

  // Each Env uses the env_allocator for its own allocations.
  Allocator env_allocator;

  void init(Allocator pool_allocator, Allocator env_allocator) {
    pool.init(pool_allocator);
    this->env_allocator = env_allocator;
  }

  void deinit() {
    pool.deinit();
    memset(this, 0, sizeof(*this));
  }

  Env<T> *alloc(Env<T> *parent) {
    Env<T> *env = pool.alloc();
    env->init(env_allocator, parent);
    return env;
  }

  void dealloc(Env<T> *env) {
    env->deinit();
    pool.dealloc(env);
  }
};
