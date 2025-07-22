#pragma once

#include "toteload.hh"

struct StrKey {
  u32 idx;
};

ttld_inline b32 str_eq_with_context(void *context, Str a, Str b) { return str_eq(a, b); }
u32 str_hash(void *context, Str s);

struct StringInterner {
  Allocator storage;
  HashMap<Str, StrKey, str_eq_with_context, str_hash> map;
  Vector<Str> list;

  void init(Allocator storage_allocator, Allocator map_allocator, Allocator list_allocator);
  void deinit();

  StrKey add(Str s);
  Str get(StrKey key) { return list[key.idx]; }
};

