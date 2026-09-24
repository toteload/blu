#ifndef RESOLVER_H
#define RESOLVER_H

#include "blu.h"

#define MAX_RESOLVE_DEPTH 64

typedef struct {
  Declaration *decl;
  RunState state;
  u8 min_required_resolve_status;
} ResolveEntry;

typedef struct {
  b32 ok;

  MessageSink *msg_sink;

  // List of user defined declarations.
  u32 user_declaration_count;
  Declaration **user_declarations;

  DeclarationInterner *decls;

  // pool of run states that map declarations to runstates. if a declaration has been resolved the runstate can be freed.

  Specializer *in;

  Stack(ResolveEntry) resolve_stack;
} Resolver;

typedef struct {
  MessageSink *msg_sink;
  DeclarationInterner *decls;
  Specializer *sp;

  u32 decls_to_resolve_count;
  Declaration **decls_to_resolve;
} ResolverOptions;

void resolver_init(Resolver *resolver, Declarations *decls);
void resolver_deinit(Resolver *resolver);

b32 resolve_declarations(Resolver *resolver);

#endif // RESOLVER_H
