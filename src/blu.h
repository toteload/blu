#ifndef BLU_H
#define BLU_H

#include "toteload.h"

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

#endif // BLU_H
