#pragma once

#include "toteload.hh"

struct StrKey {
  u32 idx;
};

ttld_inline b32 str_eq_with_context(void *context, Str a, Str b) { return str_eq(a, b); }
u32 str_hash(void *context, Str s);

// This idea is probably not be worth the headache, but it saves a bit memory.
// You could _not_ intern the strings and make the StringInterner more of a StringDeduper.
// It is then the responsibility of the caller to make sure that the added string lives at least
// as long as the StringDeduper.

struct StringInterner {
  Allocator storage;
  HashMap<Str, StrKey, str_eq_with_context, str_hash> map;
  Vector<Str> list;

  void init(Allocator storage_allocator, Allocator map_allocator, Allocator list_allocator);
  void deinit();

  StrKey add(Str s);
  Str get(StrKey key) { return list[key.idx]; }
};

