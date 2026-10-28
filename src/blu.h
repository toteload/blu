#ifndef BLU_H
#define BLU_H

#include "toteload.h"

#define MAX_MODULE_DEPTH 16 

typedef u32 TokenOffset;
typedef u32 InstructionOffset;

typedef u32 AstIndex;

#define AST_INDEX_SOURCE 1

#define KEY_NAME TypeKey
#define KEY_FUNCTION_PREFIX type_key
#include "key.h"

#define KEY_NAME SourceKey
#define KEY_FUNCTION_PREFIX source_key
#include "key.h"

#define KEY_NAME ResidualFunctionKey
#define KEY_FUNCTION_PREFIX residual_function_key
#include "key.h"

#define KEY_NAME StringKey
#define KEY_FUNCTION_PREFIX string_key
#include "key.h"

#define KEY_NAME ValueKey
#define KEY_FUNCTION_PREFIX value_key
#include "key.h"

#define KEY_NAME DeclarationKey
#define KEY_FUNCTION_PREFIX declaration_key
#include "key.h"

#define KEY_NAME StubKey
#define KEY_FUNCTION_PREFIX stub_key
#include "key.h"

#define key_is_nil(K) _Generic((K), \
  TypeKey : type_key_is_nil, \
  SourceKey : source_key_is_nil, \
  ResidualFunctionKey : residual_function_key_is_nil, \
  StringKey : string_key_is_nil, \
  ValueKey : value_key_is_nil, \
  DeclarationKey : declaration_key_is_nil)(K)

#define key_is_some(K) _Generic((K), \
  TypeKey : type_key_is_some, \
  SourceKey : source_key_is_some, \
  ResidualFunctionKey : residual_function_key_is_some, \
  StringKey : string_key_is_some, \
  ValueKey : value_key_is_some, \
  DeclarationKey : declaration_key_is_some)(K)

#define key_eq(K,M) _Generic((K), \
  TypeKey : type_key_eq, \
  SourceKey : source_key_eq, \
  ResidualFunctionKey : residual_function_key_eq, \
  StringKey : string_key_eq, \
  ValueKey : value_key_eq, \
  DeclarationKey : declaration_key_eq)(K,M)

typedef struct ValueStore ValueStore;
typedef struct SourceAllocator SourceAllocator;
typedef struct Source Source;
typedef struct Declaration Declaration;
typedef struct Compiler Compiler;

typedef struct {
  struct {
    ValueKey type;
    ValueKey nil;
    ValueKey _bool;
    ValueKey never;
    ValueKey i8;
    ValueKey i16;
    ValueKey i32;
    ValueKey i64;
    ValueKey u8;
    ValueKey comptime_int;
    ValueKey usize;

    ValueKey _true;
    ValueKey _false;
  } val;
} Common;

// `out` must point to enough memory for the decoded string.
// The caller may assume that the amount of memory needed for the decoded string is equal to or less
// than the size of `literal`.
void decode_string_literal(String literal, u8 *out, u32 *len);

i64 read_int_sign_extend(u16 bitwidth, void *payload);
u64 read_int_zero_extend(u16 bitwidth, void *payload);

#endif // BLU_H
