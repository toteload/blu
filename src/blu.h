#ifndef BLU_H
#define BLU_H

#include "toteload.h"

#define Max_module_depth 8

typedef u32 TokenIndex;  // Offset, not optional
typedef u32 AstIndex;    // Optional, 0 means nil
typedef u32 TypeIndex;   // Optional, 0 means nil
typedef u32 StringIndex; // Optional, 0 means nil
typedef u32 InstructionIndex; // Optional, 0 means nil
typedef u32 SourceIndex;
typedef u32 ValueIndex;
typedef u32 DeclarationIndex;

typedef struct EnvAllocator EnvAllocator;
typedef struct Env Env;
typedef struct ValueStore ValueStore;
typedef struct SourceAllocator SourceAllocator;
typedef struct Source Source;

#endif // BLU_H
