#ifndef BLU_H
#define BLU_H

#include "toteload.h"

typedef u32 StringIndex;
typedef u32 TypeIndex;
typedef u32 TokenIndex;
typedef u32 AstIndex;
typedef u32 ValueIndex;

enum NodeIndexKind {
  NodeIndex_none,
  NodeIndex_ast,
  NodeIndex_value,
};

typedef struct {
  u8 kind;

  union {
    AstIndex   ast;
    ValueIndex value;
  } idx;
} NodeIndex;

typedef struct Messages Messages;
typedef struct StringInterner StringInterner;
typedef struct TypeInterner TypeInterner;
typedef struct EnvAllocator EnvAllocator;
typedef struct Env Env;
typedef struct ValueStore ValueStore;

#include "tokens.h"
#include "ast.h"

typedef struct {
  String   filename;
  String   text;
  Tokens   tokens;
  AstNodes ast;
} SourceFile;

#endif // BLU_H
