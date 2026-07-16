#ifndef CODEGEN_H
#define CODEGEN_H

#include "blu.h"

typedef struct {
  Arena *arena;
  Arena *scratch;

  Common         *common;
  MessageSink    *msg_sink;
  StringInterner *strings;
  DeclarationInterner *decl_keys;
  ValueStore          *values;
} CodeGenContext;

b32 source_generate_code(CodeGenContext *context, Source *source, u32 idx_declaration);

#endif // CODEGEN_H
