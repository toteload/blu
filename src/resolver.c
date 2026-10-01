#include "blu.h"
#include "resolver.h"
#include "specialize.h"

typedef struct {
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
#define SEGMENTLIST_OUTPUT_DECLARATIONS
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

#define FUNCTION_STACK_MIN_SIZE_LOG_2 8 // 256
#define FUNCTION_STACK_SEGMENT_COUNT  16 // 16M total
#define FUNCTION_STACK_NAME FunctionStack
#define FUNCTION_STACK_TYPE ValueIndex
#define SEGMENTLIST_NAME FUNCTION_STACK_NAME
#define SEGMENTLIST_TYPE FUNCTION_STACK_TYPE
#define SEGMENTLIST_MIN_SIZE_LOG2 FUNCTION_STACK_MIN_SIZE_LOG_2
#define SEGMENTLIST_SEGMENT_COUNT FUNCTION_STACK_SEGMENT_COUNT
#define SEGMENTLIST_FUNCTION_PREFIX function_stack
#define SEGMENTLIST_OUTPUT_DECLARATIONS
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

typedef struct {
  b32 has_error;

  ResolveStack resolve_stack;

  // Order in which functions get resolved is irrelevant, but a stack is simple.
  FunctionStack function_stack;
} Resolver;

internal void push_resolve_entry(Resolver *resolver, Declaration *decl) {
  ResolveEntry *entry = resolve_stack_push(&resolver->resolve_stack, resolver->scratch);
  specializer_state_init_decl(&entry->state, resolver->sp.scratch, decl);
}

internal b32 resolve_entry(Resolver *resolver) {
  ResolveEntry *entry = stack_peek_ptr_unchecked(&resolver->resolve_stack);
  Declaration *decl = entry->state.decl;
  Frame *f = &entry->state.frame;

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
    push_resolve_entry(resolver, res.data.decl);
    return True;
  }
  case Run_register_function: {
    function_stack_append(&resolver->function_stack, res.data.function);
    return True;
  }
  }

  Unreachable();
}

internal b32 resolve_function(Resolver *resolver, ValueResidualFunction *func) {
  Todo();
}

internal void clear_resolver_with_error(Resolver *resolver) {
  resolver->has_error = False;

  while (!stack_is_empty(&resolver->resolve_stack)) {
    ResolveEntry *entry = stack_peek_ptr_unchecked(&resolver->resolve_stack);
    entry->state.decl->resolve_status = ResolveStatus_error;
    pop_resolve_entry(resolver);
  }
}

b32 resolve_declarations(ResolveContext *ctx, u32 decls_to_resolve_count, Declaration **decls_to_resolve) {
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
    Declaration *decl = ctx->decls_to_resolve[i];

    if (decl->resolve_status != ResolveStatus_unresolved) {
      continue;
    }

    ArenaSnapshot snapshot = arena_scope_begin(ctx->scratch);

    push_resolve_entry(&resolver, decl);

    while (!resolve_stack_is_empty(&resolver.resolve_stack)) {
      ResolveEntry *entry = resolve_stack_peek_ptr_unchecked(&resolver.resolve_stack);

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

          clear_resolve_stack_with_error(&resolver);
          continue;
        }

        break;
      }
      case ResolveStatus_fully_resolved: {
        resolve_stack_pop(&resolver.resolve_stack);
        continue;
      }
      case ResolveStatus_error: {
        clear_resolve_stack_with_error(&resolver);
        continue;
      }
      }

      b32 ok = resolve_entry(&resolver);
      if (!ok) {
        clear_resolve_stack_with_error(&resolver);
      }
    }

    while (!function_stack_is_empty(&resolver.function_stack)) {
      ValueIndex vidx = function_stack_peek_ptr_unchecked(&resolver.function_stack);
      Value *v = values_get(values, vidx);
      ValueResidualFunction *func = v->data;

      switch (Cast(ResidualFunctionStatus, func->status)) {
      case ResidualFunctionStatus_nil: break;
      case ResidualFunctionStatus_building:
      case ResidualFunctionStatus_finished:
      case ResidualFunctionStatus_error:
        Panic(); // This should never happen
      }

      b32 ok = resolve_function(&resolver, func);
      if (!ok) {
        Todo();
      }

      function_stack_pop(&resolver.function_stack);
    }

    zero_struct(ResolveStack, &resolver.resolve_stack);
    zero_struct(FunctionStack, &resolver.function_stack);

    arena_scope_end(ctx->scratch, snapshot);
  }

  return resolver.ok;
}
