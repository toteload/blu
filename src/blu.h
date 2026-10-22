#ifndef BLU_H
#define BLU_H

#include "toteload.h"

#define MAX_MODULE_DEPTH 16 

#define AstIndex_source 1

typedef u32 TokenIndex;       // Offset, not optional
typedef u32 InstructionIndex; // Offset, not optional

#define KEY_NAME TypeKey
#define KEY_FUNCTION_PREFIX type_key
#include "key.h"

#define key_is_nil(K) _Generic((K), \
  TypeKey : type_key_is_nil)(K)

#define key_is_some(K) _Generic((K), \
  TypeKey : type_key_is_some)(K)

#define key_eq(K,M) _Generic((K), \
  TypeKey : type_key_eq)(K,M)

typedef u32 AstIndex;         // Optional, 0 means nil
typedef u32 StringIndex;      // Optional, 0 means nil
typedef u32 ValueIndex;       // Optional, 0 means nil
typedef u32 SourceIndex;      // Optional, 0 means nil
typedef u32 DeclarationIndex; // Optional, 0 means nil or root

typedef struct ValueStore ValueStore;
typedef struct SourceAllocator SourceAllocator;
typedef struct Source Source;
typedef struct Declaration Declaration;

typedef struct {
  struct {
    ValueIndex type;
    ValueIndex nil;
    ValueIndex _bool;
    ValueIndex never;
    ValueIndex i8;
    ValueIndex i16;
    ValueIndex i32;
    ValueIndex i64;
    ValueIndex u8;
    ValueIndex comptime_int;
    ValueIndex usize;

    ValueIndex _true;
    ValueIndex _false;
  } val;
} Common;

// `out` must point to enough memory for the decoded string.
// The caller may assume that the amount of memory needed for the decoded string is equal to or less
// than the size of `literal`.
void decode_string_literal(String literal, u8 *out, u32 *len);

i64 read_int_sign_extend(u16 bitwidth, void *payload);
u64 read_int_zero_extend(u16 bitwidth, void *payload);

#endif // BLU_H
