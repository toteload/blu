#include "blu.h"
#include "resolver.h"
#include "specialize.h"

typedef enum {
  ResolveEntryKind_declaration,
  ResolveEntryKind_function,
} ResolveEntryKind;

typedef struct {
  u8 kind;
  ValueIndex stub;
  SpecializerState state;
} ResolveEntry;

#define RESOLVE_STACK_MIN_SIZE_LOG_2 8
#define RESOLVE_STACK_SEGMENT_COUNT  16
#define RESOLVE_STACK_NAME ResolveStack
#define RESOLVE_STACK_TYPE ResolveEntry
#define SEGMENTLIST_NAME RESOLVE_STACK_NAME
#define SEGMENTLIST_TYPE RESOLVE_STACK_TYPE
#define SEGMENTLIST_MIN_SIZE_LOG2 RESOLVE_STACK_MIN_SIZE_LOG_2
#define SEGMENTLIST_SEGMENT_COUNT RESOLVE_STACK_SEGMENT_COUNT
#define SEGMENTLIST_FUNCTION_PREFIX resolve_stack
#define SEGMENTLIST_OUTPUT_TYPES
#define SEGMENTLIST_OUTPUT_DECLARATIONS
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

typedef struct {
  ResolveStack primary_stack;
  ResolveStack deferred_stack;
} Resolver;

internal void push_resolve_decl_entry(ResolveContext *ctx, ResolveStack *stack, DeclarationIndex decl) {
  Declaration *d = decls_get(ctx->decls, decl);

  ResolveEntry *entry = resolve_stack_push(stack, ctx->scratch);

  entry->kind = ResolveEntryKind_declaration;
  entry->stub = d->data.decl.val;

  specializer_state_init(&entry->state, ctx->scratch);
  frame_init(&entry->state.frame, ctx->scratch, d->data.decl.chunk);
  push_scope(&entry->state.frame, Scope_comptime, 0, d->data.decl.chunk.opcode_count);
}

internal void push_resolve_function_entry(ResolveContext *ctx, ResolveStack *stack, ValueIndex stub) {
  Value *v = values_get(ctx->values, stub);
  ValueStub *stub = v->data;

  ResolveEntry *entry = resolve_stack_push(stack, ctx->scratch);

  

  Todo();
}

internal b32 try_resolve_entry(ResolveContext *ctx, Resolver *resolver) {
  ResolveEntry *entry = stack_peek_ptr_unchecked(&resolver->resolve_stack);

  switch (Cast(ResolveEntryKind, entry->source.kind)) {
  case ResolveEntryKind_declaration: {
    Todo();
    Declaration *decl = decls_get(ctx->decls, entry->source.decl);
    Frame *f = &entry->state.frame;
  } break;
  case ResolveEntryKind_function: {
    Todo();
  } break;
  case ResolveEntryKind_type: {
    Todo();
  } break;
  }

  if (!entry->state.requested_resolution) {
    Assert(decl->resolve_status == ResolveStatus_unresolved);
    decl->resolve_status = ResolveStatus_resolving_value;
  }

  RunResult res = run_toplevel_block(&resolver->sp, &entry->state);

  switch (Cast(RunResultCode, res.code)) {
  case Run_error:
    return False;
  case Run_ok: {
    decl->data.decl.val = res.data.val;
    decl->resolve_status = ResolveStatus_fully_resolved;
    return True;
  }
  case Run_resolve_declaration_value: {
    push_resolve_decl_entry(resolver, res.data.decl);
    return True;
  }
  case Run_register_function: {
    push_resolve_function_entry(&resolver->deferred_stack, res.data.register_function.stub);
    return True;
  }
  }

  Unreachable();
}

internal void clear_resolver_with_error(Resolver *resolver) {
  while (!stack_is_empty(&resolver->resolve_stack)) {
    ResolveEntry *entry = stack_peek_ptr_unchecked(&resolver->resolve_stack);
    entry->state.decl->resolve_status = ResolveStatus_error;
    pop_resolve_entry(resolver);
  }
}

internal b32 resolve_primary_stack(Resolver *resolver) {
  while (!resolve_stack_is_empty(&resolver->resolve_stack)) {
    ResolveEntry *entry = resolve_stack_peek_ptr_unchecked(&resolver->resolve_stack);

    switch (entry->state.decl->resolve_status) {
    case ResolveStatus_unresolved: break;
    case ResolveStatus_resolving_value: {
      if (!entry->state.requested_resolution) {
        Message_error(
          ctx->msg_sink,
          (MessageLocation){
            .kind = MessageLocation_unspecified,
            .decl_idx = entry->state.decl->idx,
          },
          string_lit("Encountered circular declaration")
        );

        clear_resolve_stack_with_error(resolver);

        return False;
      }

      break;
    }
    case ResolveStatus_fully_resolved: {
      resolve_stack_pop(&resolver->resolve_stack);
      continue;
    }
    case ResolveStatus_error: {
      clear_resolve_stack_with_error(resolver);
      return False;
    }
    }

    b32 ok = try_resolve_entry(&resolver);
    if (!ok) {
      clear_resolve_stack_with_error(resolver);
      return False;
    }
  }

  return True;
}

b32 resolve_declarations(ResolveContext *ctx, u32 decls_to_resolve_count, DeclarationIndex *decls_to_resolve) {
  Resolver resolver = { 0 };

  Specializer sp = {
    .perm = ctx->perm,
    .scratch = ctx->scratch,
    .msg_sink = ctx->msg_sink,
    .declarations = ctx->decls,
    .types = ctx->types,
    .values = ctx->values,
    .common = ctx->common,
  };

  for (u32 i = 0; i < ctx->decls_to_resolve_count; i++) {
    Declaration *decl = decls_get(ctx->decls, decls_to_resolve[i]);

    if (decl->resolve_status != ResolveStatus_unresolved) {
      continue;
    }

    ArenaSnapshot snapshot = arena_scope_begin(ctx->scratch);

    push_resolve_decl_entry(&resolver, decl);

    b32 ok = resolve_primary_stack(&resolver);
    if (!ok) {
      Todo();
    }

    while (!resolve_stack_is_empty(&resolver.deferred_stack)) {
      ResolveEntry entry = resolve_stack_pop(&resolver.deferred_stack);
      resolve_stack_append(&resolver.primary_stack, entry);

      b32 ok = resolve_primary_stack(&resolver);
      if (!ok) {
        Todo();
      }
    }

    zero_struct(ResolveStack, &resolver.resolve_stack);
    zero_struct(FunctionStack, &resolver.function_stack);

    arena_scope_end(ctx->scratch, snapshot);
  }

  return resolver.ok;
}
