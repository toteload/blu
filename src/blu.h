#ifndef BLU_H
#define BLU_H

#include "toteload.h"

#define Max_module_depth 8

#define AstIndex_source 1

typedef u32 TokenIndex;       // Offset, not optional
typedef u32 InstructionIndex; // Offset, not optional

typedef u32 AstIndex;         // Optional, 0 means nil
typedef u32 TypeIndex;        // Optional, 0 means nil
typedef u32 StringIndex;      // Optional, 0 means nil
typedef u32 ValueIndex;       // Optional, 0 means nil
typedef u32 SourceIndex;      // Optional, 0 means nil
typedef u32 DeclarationIndex; // Optional, 0 means nil or root
typedef u32 ResidualFunctionKey; // Optional, 0 means nil

typedef struct ValueStore ValueStore;
typedef struct SourceAllocator SourceAllocator;
typedef struct Source Source;
typedef struct Declaration Declaration;

typedef struct {
  struct {
    TypeIndex comptime_int;
    TypeIndex type;
    TypeIndex nil;
    TypeIndex bool;
    TypeIndex never;
    TypeIndex u8;
    TypeIndex i8;
    TypeIndex i16;
    TypeIndex i32;
    TypeIndex i64;
    TypeIndex usize;
  } type;

  struct {
    ValueIndex type;
    ValueIndex nil;
    ValueIndex bool;
    ValueIndex never;
    ValueIndex i8;
    ValueIndex i16;
    ValueIndex i32;
    ValueIndex i64;
    ValueIndex u8;
    ValueIndex comptime_int;
    ValueIndex usize;

    ValueIndex true;
    ValueIndex false;
  } val;
} Common;

// `out` must point to enough memory for the decoded string.
// The caller may assume that the amount of memory needed for the decoded string is equal to or less
// than the size of `literal`.
void decode_string_literal(String literal, u8 *out, u32 *len);

i64 read_int_sign_extend(u16 bitwidth, void *payload);
u64 read_int_zero_extend(u16 bitwidth, void *payload);

#endif // BLU_H
