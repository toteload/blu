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

typedef uint32_t b32;

// ---

#define Cat_aux(a,b) a##b
#define Cat(a,b) Cat_aux(a,b)

// ---

#define Try(e) if (!(e)) { return false; }

// ---

#define ttld_inline __attribute__((always_inline)) inline

// ---

template<typename T>
constexpr void swap(T &a, T &b) {
  T tmp = a;
  a = b;
  b = tmp;
}

template<typename T>
constexpr T min(T a, T b) {
  if (a < b) {
    return a;
  }

  return b;
}

template<typename T>
constexpr T max(T a, T b) {
  if (a > b) {
    return a;
  }

  return b;
}

template<typename T, typename U>
constexpr T cast(U &&x) {
  return (T)x;
}

#define Is_null(p) ((p) == nullptr)

#define Align_of(x) _Alignof(x)

#define KiB(x) (Cast(u64,x) << 10)
#define MiB(x) (Cast(u64,x) << 20)
#define GiB(x) (Cast(u64,x) << 30)

#define Cast(T, x) ((T)(x))

#define Round_up_to_power_of_two(x, p) (((x) + ((p)-1)) & (~((p)-1)))

template<typename T>
constexpr b32 is_zero_or_power_of_two(T x) {
  return ((((x)-1) & (x)) == 0);
}

#define Ptr_forward_align(p,a) Cast(void*, Round_up_to_power_of_two(Cast(u64, p), Cast(u64,a)))
#define Ptr_offset(p,d)        Cast(void*, Cast(u8*,p)+d)
#define Ptr_diff(a,b)          (Cast(u8*,a) - Cast(u8*,b))

#ifdef TTLD_DEBUG
#define Debug_assert(cond) assert(cond)
#else
#define Debug_assert(cond) Cast(void, cond)
#endif

#define Panic()       abort()
#define Unreachable() Panic()
#define Todo()        Panic()
#define Unimplemented assert(!"Unimplemented");

// ---

#define EachIndex(i, count) (u64 i = 0; i < (count); i += 1)

// ---

struct Str {
  char const *str;
  u32 len;
  u32 _pad;
};

#define Str_make(s) { s, .len = sizeof(s)-1, }

ttld_inline
b32 str_eq(Str a, Str b) {
  b32 is_same_len = a.len == b.len;
  b32 is_same_content = memcmp(a.str, b.str, a.len) == 0;
  return is_same_len && is_same_content;
}

template<typename T>
struct Slice {
  T *data;
  usize len;

  T &operator[](usize idx) {
    return data[idx];
  }
};

// @os

namespace ttld::os {
u32 page_size();

void *mem_reserve(u64 size);
b32   mem_commit(void *p, u64 size);
void  mem_release(void *p, u64 size);
}

// @allocator

typedef void *(*AllocatorFunction)(
  void *ctx, void *ptr, usize old_byte_size, usize new_byte_size, u32 align
);

constexpr u32 default_align = 8;

struct Allocator {
  AllocatorFunction fn;
  void *ctx;

  ttld_inline void *raw_alloc(usize byte_size, u32 align=default_align);
  ttld_inline void *raw_realloc(void *p, usize byte_size, usize new_byte_size, u32 align=default_align);
  ttld_inline void raw_free(void *p, usize byte_size);

  template<typename T>
  T *alloc(usize count = 1);

  template<typename T>
  T *realloc(T *p, usize old_count, usize new_count);

  template<typename T>
  void free(T *p, usize count);
};

void *Allocator::raw_alloc(usize byte_size, u32 align) {
  return fn(ctx, nullptr, 0, byte_size, align);
}

void *Allocator::raw_realloc(void *p, usize byte_size, usize new_byte_size, u32 align) {
  return fn(ctx, p, byte_size, new_byte_size, align);
}

void Allocator::raw_free(void *p, usize byte_size) {
  fn(ctx, p, byte_size, 0, 0);
}

template<typename T>
T *Allocator::alloc(usize count) {
  return Cast(T*, raw_alloc(count * sizeof(T), Align_of(T)));
}

template<typename T>
T *Allocator::realloc(T *p, usize count, usize new_count) {
  return Cast(T*, raw_realloc(p, count * sizeof(T), new_count * sizeof(T), Align_of(T)));
}

template<typename T>
void Allocator::free(T *p, usize count) {
  raw_free(p, count * sizeof(T));
}

// @arena

struct Arena;

struct ArenaSnapshot {
  Arena *owner;
  void  *at;
};

struct Arena {
  void *reserve_end;
  void *commit_end;
  void *at;
  void *base;

  void init(usize reserve_size);
  void deinit();

  Allocator as_allocator();

  template<typename T>
  T *alloc(usize count=1);

  ttld_inline ArenaSnapshot take_snapshot();
  void restore(ArenaSnapshot snapshot);
};

ttld_inline
ArenaSnapshot Arena::take_snapshot() {
  return { .owner = this, .at = at };
}

ttld_inline
void Arena::restore(ArenaSnapshot snapshot) {
  Debug_assert(this == snapshot.owner);
  at = snapshot.at;
}

