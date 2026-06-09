#pragma once

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

typedef float  f32;
typedef double f64;

typedef int8_t      i8;
typedef uint8_t     u8;
typedef int16_t     i16;
typedef uint16_t    u16;
typedef int32_t     i32;
typedef uint32_t    u32;
typedef int64_t     i64;
typedef uint64_t    u64;

typedef uintptr_t usize;
typedef intptr_t  isize;

typedef uint8_t  b8;
typedef uint16_t b16;
typedef uint32_t b32;
typedef uint64_t b64;

#ifdef __clang__
#define TTLD_COMPILER_CLANG 1
#endif

#ifdef _WIN32
#define TTLD_OS_WINDOWS 1
#endif

#ifdef __APPLE__
#define TTLD_OS_MACOS 1
#endif

#define Cat_(a, b) a##b
#define Cat(a, b) Cat_(a, b)

#define internal      static
#define always_inline __attribute__((always_inline)) inline

#define Cast(T, x) ((T)(x))

always_inline u32 bitwidth(u64 x) {
#ifdef TTLD_COMPILER_CLANG
  if (x == 0) {
    return 0;
  }
  return 64 - Cast(u32, __builtin_clz(x));
#else
#error "todo: bitwidth is not implemented for this platform"
#endif
}

#define KiB(x) (Cast(u64, x) << 10)
#define MiB(x) (Cast(u64, x) << 20)
#define GiB(x) (Cast(u64, x) << 30)

#define Swap(Type, a, b) do { \
  Type tmp_ = (a); \
  a = (b); \
  b = tmp_; \
} while (0)

#define Max(a, b) (((a)>(b))?(a):(b))
#define Min(a, b) (((a)<(b))?(a):(b))

#define EachIndex(i, count) (usize i = 0; i < (count); i += 1)

#define Null NULL
#define True 1
#define False 0

#define is_null(p) ((p) == Null)
#define Unused(x) ((void)(x))

always_inline b32 is_zero_or_power_of_two(usize x) {
  return ((((x)-1) & (x)) == 0);
}

always_inline usize round_up_to_power_of_two(usize x, usize power) {
  return (x + (power - 1)) & (~(power - 1));
}

always_inline void *ptr_offset(void const *p, usize offset) {
  return Cast(void*, Cast(u8 const*, p) + offset);
}

always_inline isize ptr_diff(void const *a, void const *b) {
  return Cast(isize, Cast(u8*, a) - Cast(u8*, b));
}

always_inline void *ptr_forward_align(void const *p, u32 align) {
  return Cast(void*, round_up_to_power_of_two(Cast(usize, p), align));
}

#define zero_struct(type, p) memset(p, 0, sizeof(type))

#define Align_of(x) _Alignof(x)

#define Assert(e) assert(e)
#define Panic(Reason) abort()
#define Unreachable() Panic()

typedef struct String {
  u8 const *str;
  usize len;
} String;

#define string_lit(s) \
  _Pragma("clang diagnostic push") \
  _Pragma("clang diagnostic ignored \"-Wpointer-sign\"") \
  ((String){ .str = s, .len = (sizeof(s) - 1), }) \
  _Pragma("clang diagnostic pop")

always_inline b32 string_eq(String a, String b) {
  if (a.len != b.len) {
    return False;
  }

  return memcmp(a.str, b.str, a.len) == 0;
}

typedef void *(*AllocatorFunction)(
  void *ctx, void *ptr, size_t old_byte_size, size_t new_byte_size, u32 align
);

typedef struct Allocator {
  AllocatorFunction fn;
  void *ctx;
} Allocator;

#define Alloc(allocator, size, align)                      (allocator).fn((allocator).ctx, Null, 0, size, align)
#define Realloc(allocator, ptr, old_size, new_size, align) (allocator).fn((allocator).ctx, ptr, old_size, new_size, align)
#define Free(allocator, ptr, size)                         (allocator).fn((allocator).ctx, ptr, size, 0, 0)

usize vmem_page_size(void);
void *vmem_reserve(usize size);
b32   vmem_commit(void *p, usize size);
void  vmem_release(void *p, usize size);

typedef struct {
  void *base;
  void *commit_end;
  void *reserve_end;
  void *at;
} Arena;

typedef struct {
  Arena *arena;
  void  *at;
} ArenaSnapshot;

typedef struct {
  usize reserve_size;
  usize initial_commit_size;
} ArenaOptions;

void arena_init(Arena *arena, ArenaOptions *options);
void arena_deinit(Arena *arena);

void *arena_push(Arena *arena, usize size, u32 align);

#define arena_push_array(arena, type, count) arena_push(arena, (count) * sizeof(type), Align_of(type))

String arena_copy_string(Arena *arena, String s);

always_inline ArenaSnapshot arena_scope_begin(Arena *arena) {
  return (ArenaSnapshot){ .arena = arena, .at = arena->at, };
}

always_inline void arena_scope_end(Arena *arena, ArenaSnapshot snapshot) {
  Assert(arena == snapshot.arena);
  arena->at = snapshot.at;
}

