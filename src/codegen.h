#ifndef CODEGEN_H
#define CODEGEN_H

#include "blu.h"

typedef struct {
  Arena *perm; // The generated code is stored in this arena
  Arena *scratch;
  Common         *common;
  MessageSink    *msg_sink;
  StringInterner *strings;
  DeclarationInterner *decls;
  ValueStore          *values;
} CodeGenContext;

b32 generate_code(CodeGenContext *context, Declaration *decl);

#endif // CODEGEN_H
