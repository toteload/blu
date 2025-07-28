#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

// ---

typedef float f32;
typedef double f64;

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;

typedef uintptr_t usize;
typedef intptr_t isize;

typedef uint32_t b32;

// ---

#define Cat_aux(a, b) a##b
#define Cat(a, b) Cat_aux(a, b)

// ---

#define Try(e)                                                                                     \
  if (!(e)) {                                                                                      \
    return false;                                                                                  \
  }

// ---

#define ttld_inline __attribute__((always_inline)) inline

// ---

template<typename T> constexpr void swap(T &a, T &b) {
  T tmp = a;
  a     = b;
  b     = tmp;
}

template<typename T> constexpr T min(T a, T b) {
  if (a < b) {
    return a;
  }

  return b;
}

template<typename T> constexpr T max(T a, T b) {
  if (a > b) {
    return a;
  }

  return b;
}

template<typename T, typename U> constexpr T cast(U &&x) { return (T)x; }

#define Is_null(p) ((p) == nullptr)

#define Align_of(x) _Alignof(x)

#define KiB(x) (Cast(u64, x) << 10)
#define MiB(x) (Cast(u64, x) << 20)
#define GiB(x) (Cast(u64, x) << 30)

#define Cast(T, x) ((T)(x))

template<typename T> constexpr T round_up_to_power_of_two(T x, T p) {
  return (x + (p - 1)) & (~(p - 1));
}
template<typename T> constexpr b32 is_zero_or_power_of_two(T x) { return ((((x)-1) & (x)) == 0); }

template<typename T> constexpr void *ptr_forward_align(T *p, u32 align) {
  return cast<void *>(round_up_to_power_of_two(cast<usize>(p), cast<usize>(align)));
}

template<typename T, typename U> isize ptr_diff(T *a, U *b) {
  return cast<u8 *>(a) - cast<u8 *>(b);
}

template<typename T> constexpr void *ptr_offset(T *p, isize d) { return cast<u8 *>(p) + d; }

#ifdef TTLD_DEBUG
#define Debug_assert(cond) assert(cond)
#else
#define Debug_assert(cond) Cast(void, cond)
#endif

#define Panic() abort()
#define Unreachable() Panic()
#define Todo() assert(!"TODO")
#define Unimplemented() assert(!"Unimplemented")

// ---

#define ForEachIndex(i, count) for (usize i = 0; i < (count); i += 1)

// ---

struct CStr {};

struct Str {
  char const *str = nullptr;
  usize _len      = 0;

  bool is_ok() { return str && _len; }
  bool is_empty() { return _len == 0; }
  char const *end() { return str + _len; }

  char operator[](usize idx) { return str[idx]; }

  usize len() { return _len; }

  static Str empty() { return {}; }

  static Str from_cstr(char const *s) {
    // We don't include the null terminator in the length
    usize len = strlen(s);
    return {s, len};
  }
};

#define Str_make(s)                                                                                \
  { s, (sizeof(s) - 1), }

ttld_inline b32 str_eq(Str a, Str b) {
  b32 is_same_len     = a.len() == b.len();
  b32 is_same_content = memcmp(a.str, b.str, a.len()) == 0;
  return is_same_len && is_same_content;
}

template<typename T> struct Slice {
  T *data;
  usize _len;

  T &operator[](usize idx) { return data[idx]; }
  usize len() { return _len; }
  T *end() { return data + _len; }
};

// @os

namespace ttld::os
{
u32 page_size();

void *mem_reserve(usize size);
b32 mem_commit(void *p, usize size);
void mem_release(void *p, usize size);
} // namespace ttld::os

// @allocator

typedef void *(*AllocatorFunction)(
  void *ctx, void *ptr, usize old_byte_size, usize new_byte_size, u32 align
);

constexpr u32 default_align = 8;

struct Allocator {
  AllocatorFunction fn;
  void *ctx;

  ttld_inline void *raw_alloc(usize byte_size, u32 align = default_align);
  ttld_inline void *
  raw_realloc(void *p, usize byte_size, usize new_byte_size, u32 align = default_align);
  ttld_inline void raw_free(void *p, usize byte_size);

  template<typename T> T *alloc(usize count = 1);

  template<typename T> T *realloc(T *p, usize old_count, usize new_count);

  template<typename T> void free(T *p, usize count);
};

void *Allocator::raw_alloc(usize byte_size, u32 align) {
  return fn(ctx, nullptr, 0, byte_size, align);
}

void *Allocator::raw_realloc(void *p, usize byte_size, usize new_byte_size, u32 align) {
  return fn(ctx, p, byte_size, new_byte_size, align);
}

void Allocator::raw_free(void *p, usize byte_size) { fn(ctx, p, byte_size, 0, 0); }

template<typename T> T *Allocator::alloc(usize count) {
  return Cast(T *, raw_alloc(count * sizeof(T), Align_of(T)));
}

template<typename T> T *Allocator::realloc(T *p, usize count, usize new_count) {
  return Cast(T *, raw_realloc(p, count * sizeof(T), new_count * sizeof(T), Align_of(T)));
}

template<typename T> void Allocator::free(T *p, usize count) { raw_free(p, count * sizeof(T)); }

// @arena

struct Arena;

struct ArenaSnapshot {
  Arena *owner;
  void *at;
};

struct Arena {
  void *reserve_end;
  void *commit_end;
  void *at;
  void *base;

  void init(usize reserve_size);
  void deinit();

  usize size() { return ptr_diff(at, base); }

  void *raw_alloc(usize byte_size, u32 align);
  template<typename T> T *alloc(usize count = 1) {
    return cast<T *>(raw_alloc(count * sizeof(T), Align_of(T)));
  }

  Str push_format_string(char const *format, ...);

  ttld_inline ArenaSnapshot take_snapshot();
  void restore(ArenaSnapshot snapshot);
};

ttld_inline ArenaSnapshot Arena::take_snapshot() { return {.owner = this, .at = at}; }

ttld_inline void Arena::restore(ArenaSnapshot snapshot) {
  Debug_assert(this == snapshot.owner);
  at = snapshot.at;
}

// -[ Object pool ]-

template<typename T> struct ObjectPool {
  union Item {
    T item;
    Item *next;
  };

  struct Block {
    Block *next = nullptr;
    u32 count;
    Item items[0];
  };

  Allocator backing;
  Block *blocks  = nullptr;
  Item *freelist = nullptr;

  void init(Allocator allocator) { backing = allocator; }
  void deinit();

  void grow(u32 count);

  T *alloc() {
    if (freelist == nullptr) {
      grow(KiB(2));
    }

    Item *p  = freelist;
    freelist = freelist->next;
    return &p->item;
  }

  void dealloc(T *p) {
    Item *item = cast<Item *>(p);
    item->next = freelist;
    freelist   = item;
  }
};

template<typename T> void ObjectPool<T>::deinit() {
  Block *at = blocks;

  while (at != nullptr) {
    at = at->next;

    backing.free(at);
  }

  blocks   = nullptr;
  freelist = nullptr;
}

template<typename T> void ObjectPool<T>::grow(u32 count) {
  Block *t =
    cast<Block *>(backing.raw_alloc(sizeof(Block) + count * sizeof(Item), Align_of(Block)));

  t->next  = blocks;
  t->count = count;

  for (usize i = 0; i < count - 1; i++) {
    t->items[i].next = &t->items[i + 1];
  }

  t->items[count - 1].next = freelist;

  blocks   = t;
  freelist = &t->items[0];
}
