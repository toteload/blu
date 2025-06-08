#include "hashmap.hh"
#include <stdio.h>
#include <stdlib.h>

void *stdlib_alloc_fn(void *ctx, void *p, usize old_byte_size, usize new_byte_size, u32 align) {
  if (!Is_null(p) && new_byte_size == 0) {
    free(p);
    return nullptr;
  }

  return realloc(p, new_byte_size);
}

ttld_inline
b32 cmp_i32(i32 a, i32 b) {
  return a == b;
}

ttld_inline
u32 hash_i32(i32 x) {
  return x;
}

int main() {
  Allocator stdlib_alloc{ stdlib_alloc_fn, nullptr };
  HashMap<i32, i32, cmp_i32, hash_i32> m;

  m.init(stdlib_alloc);

  m.insert(1, 10);
  m.insert(2, 20);
  m.insert(3, 30);
  m.insert(4, 40);
  m.insert(5, 50);

  assert(m.get(1) == 10);
  assert(m.get(2) == 20);
  assert(m.get(3) == 30);
  assert(m.get(4) == 40);
  assert(m.get(5) == 50);

  m.insert(3, 333);

  assert(m.get(3) == 333);

  assert(m.len == 5);

  m.remove_key(5);

  assert(m.len == 4);

  m.remove_key(3);

  assert(m.get(1) == 10);
  assert(m.get(2) == 20);
  assert(m.get(4) == 40);

  m.deinit();

  printf("Test finished!\n");

  return 0;
}
