#pragma once

#include <stdint.h>

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

typedef uint8_t b8;
typedef uint16_t b16;
typedef uint32_t b32;
typedef uint64_t b64;

#ifdef __clang__
#define TTLD_COMPILER_CLANG 1
#endif

#define Cat_(a, b) a##b
#define Cat(a, b) Cat_(a, b)

#define ttld_inline __attribute__((always_inline)) inline

ttld_inline u32 clz32(u32 x) {
#ifdef TTLD_COMPILER_CLANG
  // Result is undefined if x == 0
  return __builtin_clz(x);
#else
#error "todo"
#endif
}

#define swap(a, b) do { \
  typeof(a) _tmp = (a); \
  a = (b); \
  b = _tmp; \
} while (0)

#define is_null(p) ((p) == nullptr)
#define cast(T, x) ((T)(x))

typedef struct String {
  u8 *str;
  usize len;
} String;

#define String_from_literal(s) ((String){ .str = s, .len = (sizeof(s) - 1), })

usize ttld_vmem_page_size();
void *ttld_vmem_reserve(usize size);
b32   ttld_vmem_commit(void *p, usize size);
void  ttld_vmem_release(void *p, usize size);
