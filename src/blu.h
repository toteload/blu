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
typedef u32 DeclarationIndex; // 0 is reserved for the root module and 

typedef struct EnvAllocator EnvAllocator;
typedef struct Env Env;
typedef struct ValueStore ValueStore;
typedef struct SourceAllocator SourceAllocator;
typedef struct Source Source;
typedef struct Declaration Declaration;

#endif // BLU_H
