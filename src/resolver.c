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

internal always_inline ValueStub *get_stub(ResolveContext *ctx, ValueIndex val) {
  Value *v = values_get(ctx->values, val);
  return Cast(ValueStub*, v->data);
}

internal void push_resolve_decl_entry(ResolveContext *ctx, ResolveStack *stack, DeclarationIndex decl) {
  Declaration *d = decls_extra_get_ptr(ctx->decls, decl);

  ResolveEntry *entry = resolve_stack_push(stack, ctx->scratch);

  entry->kind = ResolveEntryKind_declaration;
  entry->stub = d->data.decl.val;

  zero_struct(SpecializerState, &entry->state);
  frame_init(&entry->state.frame, ctx->scratch, d);
  push_scope(&entry->state.frame, ctx->scratch, Scope_comptime, 0, d->data.decl.chunk.opcode_count);
}

internal void push_resolve_function_entry(ResolveContext *ctx, ResolveStack *stack, ValueIndex stub) {
  Value *v = values_get(ctx->values, stub);
  ValueStub *s = Cast(ValueStub*, v->data);

  Declaration *d = decls_extra_get_ptr(ctx->decls, s->decl);

  ResolveEntry *entry = resolve_stack_push(stack, ctx->scratch);

  entry->kind = ResolveEntryKind_function;
  entry->stub = stub;

  zero_struct(SpecializerState, &entry->state);

  frame_init(&entry->state.frame, ctx->scratch, d);

  SIrFunc *func = sir_chunk_extra(&d->data.decl.chunk, s->inst);

  push_scope(&entry->state.frame, ctx->scratch, Scope_block, s->inst, func->instruction_count);
}

internal b32 try_resolve_entry(ResolveContext *ctx, Resolver *resolver, Specializer *sp) {
  ResolveEntry *entry = resolve_stack_peek_ptr_unchecked(&resolver->primary_stack);
  Value *vstub = values_get(ctx->values, entry->stub);
  ValueStub *stub = Cast(ValueStub*, vstub->data);
  Declaration *decl = decls_extra_get_ptr(ctx->decls, stub->decl);

  switch (Cast(ResolveEntryKind, entry->kind)) {
  case ResolveEntryKind_declaration: {
    Frame *f = &entry->state.frame;
    if (!entry->state.requested_resolution) {
      Assert(stub->resolve_status == ResolveStatus_unresolved);
      stub->resolve_status = ResolveStatus_resolving;
    }
  } break;
  case ResolveEntryKind_function: {
    Todo();
  } break;
  }

  RunResult res = run_toplevel_block(sp, &entry->state);

  switch (Cast(RunResultCode, res.code)) {
  case Run_error:
    return False;
  case Run_ok: {
    resolve_stack_pop(&resolver->primary_stack);
    stub->resolve_status = ResolveStatus_fully_resolved;
    stub->idx = res.data.val;
    return True;
  }
  case Run_resolve_declaration_value: {
    push_resolve_decl_entry(ctx, &resolver->primary_stack, res.data.decl);
    return True;
  }
  case Run_register_function: {
    push_resolve_function_entry(ctx, &resolver->deferred_stack, res.data.stub);
    return True;
  }
  }

  Unreachable();
}

internal void clear_resolve_stack_with_error(ResolveContext *ctx, ResolveStack *stack) {
  while (!resolve_stack_is_empty(stack)) {
    ResolveEntry *entry = resolve_stack_peek_ptr_unchecked(stack);
    ValueStub *stub = get_stub(ctx, entry->stub);
    stub->resolve_status = ResolveStatus_error;
    resolve_stack_pop(stack);
  }
}

internal b32 try_resolve_primary_stack(ResolveContext *ctx, Resolver *resolver, Specializer *sp) {
  while (!resolve_stack_is_empty(&resolver->primary_stack)) {
    ResolveEntry *entry = resolve_stack_peek_ptr_unchecked(&resolver->primary_stack);
    ValueStub *stub = get_stub(ctx, entry->stub);

    switch (stub->resolve_status) {
    case ResolveStatus_unresolved: break;
    case ResolveStatus_resolving: {
      if (!entry->state.requested_resolution) {
        Message_error(
          ctx->msg_sink,
          (MessageLocation){
            .kind = MessageLocation_unspecified,
            .decl_idx = stub->decl,
          },
          string_lit("Encountered circular declaration")
        );

        clear_resolve_stack_with_error(ctx, &resolver->primary_stack);

        return False;
      }

      break;
    }
    case ResolveStatus_fully_resolved: Panic(); // This should never happen
    case ResolveStatus_error: Panic(); // This should never happen
    }

    b32 ok = try_resolve_entry(ctx, resolver, sp);

    if (!ok) {
      clear_resolve_stack_with_error(ctx, &resolver->primary_stack);
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

  b32 all_ok = True;

  for (u32 i = 0; i < decls_to_resolve_count; i++) {
    DeclarationIndex decl_idx = decls_to_resolve[i];
    Declaration *decl = decls_extra_get_ptr(ctx->decls, decl_idx);
    ValueStub *stub = get_stub(ctx, decl->data.decl.val);

    Assert(stub->resolve_status != ResolveStatus_resolving);

    if (stub->resolve_status == ResolveStatus_error || stub->resolve_status == ResolveStatus_fully_resolved) {
      continue;
    }

    ArenaSnapshot snapshot = arena_scope_begin(ctx->scratch);

    push_resolve_decl_entry(ctx, &resolver.primary_stack, decl_idx);

    b32 ok = try_resolve_primary_stack(ctx, &resolver, &sp);
    if (!ok) {
      all_ok = False;
      Todo();
    }

    while (!resolve_stack_is_empty(&resolver.deferred_stack)) {
      ResolveEntry entry = resolve_stack_pop(&resolver.deferred_stack);
      resolve_stack_append(&resolver.primary_stack, ctx->scratch, entry);

      b32 ok = try_resolve_primary_stack(ctx, &resolver, &sp);
      if (!ok) {
        all_ok = False;
        Todo();
      }
    }

    zero_struct(ResolveStack, &resolver.primary_stack);
    zero_struct(ResolveStack, &resolver.deferred_stack);

    arena_scope_end(ctx->scratch, snapshot);
  }

  return all_ok;
}

