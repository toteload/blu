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

typedef struct EnvAllocator EnvAllocator;
typedef struct Env Env;
typedef struct ValueStore ValueStore;
typedef struct SourceAllocator SourceAllocator;
typedef struct Source Source;
typedef struct Declaration Declaration;

// `out` must point to enough memory for the decoded string.
// The caller may assume that the amount of memory needed for the decoded string is equal to or less
// than the size of `literal`.
u32 decode_string_literal(String literal, u8 *out, u32 *len);

#endif // BLU_H
