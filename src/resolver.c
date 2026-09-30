#include "blu.h"
#include "resolver.h"
#include "specialize.h"

#define MAX_RESOLVE_DEPTH 64

#define QUEUE_NAME FunctionQueue
#define QUEUE_TYPE ResidualFunctionKey
#define QUEUE_MIN_SIZE_LOG2 4
#define QUEUE_OUTPUT_TYPES
#include "queue.h"

typedef struct {
  SpecializerState state;
  ArenaSnapshot snapshot;
} ResolveEntry;

typedef struct {
  b32 ok;
  Specializer sp;
  Stack(ResolveEntry) resolve_stack;
  FunctionQueue function_queue;
} Resolver;

internal void push_resolve_entry(Resolver *resolver, Declaration *decl) {
  ResolveEntry *entry = stack_push_ptr(&resolver->resolve_stack);

  entry->snapshot = arena_scope_begin(resolver->sp.scratch);
  specializer_state_init_decl(&entry->state, resolver->sp.scratch, decl);
}

internal void pop_resolve_entry(Resolver *resolver) {
  ResolveEntry entry = stack_pop(&resolver->resolve_stack);
  arena_scope_end(resolver->sp.scratch, entry.snapshot);
}

internal b32 resolve_entry(Resolver *resolver) {
  ResolveEntry *entry = stack_peek_ptr_unchecked(&resolver->resolve_stack);
  Declaration *decl = entry->state.decl;
  Frame *f = &entry->state.frame;

  if (!entry->state.requested_resolution) {
    Assert(decl->resolve_status == ResolveStatus_unresolved);
    decl->resolve_status = ResolveStatus_resolving_value;
  }

  u32 err = run_toplevel_block(&resolver->sp, &entry->state);

  if (err == Run_ok) {
    IRef ref = f->inst_map[0];
    Assert(iref_is_some_value(ref));

    decl->data.decl.val = iref_to_value(ref);
    decl->resolve_status = ResolveStatus_fully_resolved;

    return True;
  }

  if (err == Run_resolve_declaration_value) {
    ScopeSpan *s = stack_peek_ptr(&f->scopes);
    DeclarationIndex idx = sir_chunk_data(f->chunk, s->pc);

    push_resolve_entry(resolver, decls_extra_get_ptr(resolver->sp.declarations, idx));

    return True;
  }

  return False;
}

internal void clear_resolve_stack_with_error(Resolver *resolver) {
  resolver->ok = False;

  while (!stack_is_empty(&resolver->resolve_stack)) {
    ResolveEntry *entry = stack_peek_ptr_unchecked(&resolver->resolve_stack);
    entry->state.decl->resolve_status = ResolveStatus_error;
    pop_resolve_entry(resolver);
  }
}

b32 resolve_declarations(ResolveContext *ctx) {
  Resolver resolver = {
    .ok = True,
    .sp = {
      .perm = ctx->perm,
      .scratch = ctx->scratch,
      .msg_sink = ctx->msg_sink,
      .declarations = ctx->decls,
      .types = ctx->types,
      .values = ctx->values,
      .common = ctx->common,
    },
  };

  stack_init(
    &resolver.resolve_stack,
    arena_push_array(ResolveEntry, ctx->scratch, MAX_RESOLVE_DEPTH),
    MAX_RESOLVE_DEPTH
  );

  for (u32 i = 0; i < ctx->decls_to_resolve_count; i++) {
    Declaration *decl = ctx->decls_to_resolve[i];

    if (decl->resolve_status != ResolveStatus_unresolved) {
      continue;
    }

    push_resolve_entry(&resolver, decl);

    while (!stack_is_empty(&resolver.resolve_stack)) {
      ResolveEntry *entry = stack_peek_ptr_unchecked(&resolver.resolve_stack);
      u8 resolve_status = entry->state.decl->resolve_status;

      if (resolve_status == ResolveStatus_fully_resolved) {
        pop_resolve_entry(&resolver);
        continue;
      }

      if (resolve_status == ResolveStatus_error) {
        clear_resolve_stack_with_error(&resolver);
        break;
      }

      if (!entry->state.requested_resolution && resolve_status == ResolveStatus_resolving_value) {
        Message_error(
          ctx->msg_sink,
          (MessageLocation){
            .kind = MessageLocation_unspecified,
            .decl_idx = entry->state.decl->idx,
          },
          string_lit("Encountered circular declaration")
        );

        clear_resolve_stack_with_error(&resolver);
        break;
      }

      if (!resolve_entry(&resolver)) {
        clear_resolve_stack_with_error(&resolver);
      }
    }
  }

  return resolver.ok;
}
