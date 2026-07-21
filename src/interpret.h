#ifndef INTERPRET_H
#define INTERPRET_H

#include "blu.h"
#include "compiler.h"

typedef struct {
  Arena *perm;
  Arena *scratch;

  Common *common;
  MessageSink *msg_sink;
  DeclarationInterner *decls;
  ValueStore *values;
} InterpretContext;

b32 source_interpret_code(InterpretContext *context, Source *source, u32 idx_declaration);

#endif // INTERPRET_H
