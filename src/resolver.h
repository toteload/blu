#ifndef RESOLVER_H
#define RESOLVER_H

#include "blu.h"
#include "declaration_interner.h"

typedef struct {
  Arena *perm;
  Arena *scratch;
  MessageSink *msg_sink;
  DeclarationInterner *decls;
  TypeInterner *types;
  ValueStore *values;
  Common *common;
} ResolveContext;

b32 resolve_declarations(ResolveContext *ctx, u32 decls_to_resolve_count, Declaration **decls_to_resolve);

#endif // RESOLVER_H
