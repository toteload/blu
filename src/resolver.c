#include "blu.h"
#include "resolver.h"
#include "specialize.h"

typedef enum {
  ResolveEntryKind_declaration,
  ResolveEntryKind_function,
} ResolveEntryKind;

typedef struct {
  u8 kind;
  u8 is_started; // the entry has run before and is continued
  StubKey stub;
  Specializer state;
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

internal void push_resolve_decl_entry(ResolveContext *ctx, ResolveStack *stack, DeclarationKey decl) {
  Declaration *d = decls_extra_get_ptr(ctx->decls, decl);

  ResolveEntry *entry = resolve_stack_push(stack, ctx->scratch);

  entry->kind = ResolveEntryKind_declaration;
  entry->is_started = False;

  zero_struct(Specializer, &entry->state);
  frame_init(&entry->state.frame, ctx->scratch, d);
  push_scope(&entry->state.frame, ctx->scratch, Scope_comptime, 0, d->data.decl.chunk.opcode_count);
}

internal b32 try_resolve_entry(ResolveContext *ctx, Resolver *resolver, SpecializerContext *sp) {
  ResolveEntry *entry = resolve_stack_peek_ptr_unchecked(&resolver->primary_stack);
  Stub *stub = compiler_get_stub(ctx->compiler, entry->stub);

  if (!entry->is_started) {
    Assert(stub->resolve_status == RESOLVE_STATUS_UNRESOLVED);
    stub->resolve_status = RESOLVE_STATUS_RESOLVING;
    entry->is_started = True;
  }

  RunResult res = run_toplevel_block(sp, &entry->state);

  switch (Cast(RunResultCode, res.code)) {
  case Run_error:
    return False;
  case Run_ok: {
    Todo(); // set the value of the stub
    stub->resolve_status = RESOLVE_STATUS_FULLY_RESOLVED;
    resolve_stack_pop(&resolver->primary_stack);
    return True;
  }
  case Run_resolve_declaration_value: {
    push_resolve_decl_entry(ctx, &resolver->primary_stack, res.data.decl);
    return True;
  }
  case Run_register_function: {
    StubKey stub = compiler_add_stub(
      ctx->compiler,
      res.data.register_function.decl,
      res.data.register_function.inst,
      res.data.register_function.type_signature);

    ResolveEntry *entry = resolve_stack_push(&resolver->deferred_stack, sp->scratch);

    entry->kind = ResolveEntryKind_function;
    entry->is_started = False;

    specializer_init_function(sp, &entry->state, stub);

    return True;
  }
  }

  Unreachable();
}

internal void clear_resolve_stack_with_error(ResolveContext *ctx, ResolveStack *stack) {
  while (!resolve_stack_is_empty(stack)) {
    ResolveEntry *entry = resolve_stack_peek_ptr_unchecked(stack);
    Stub *stub = compiler_get_stub(ctx->compiler, entry->stub);
    stub->resolve_status = RESOLVE_STATUS_ERROR;
    resolve_stack_pop(stack);
  }
}

internal b32 try_resolve_primary_stack(ResolveContext *ctx, Resolver *resolver, SpecializerContext *sp) {
  while (!resolve_stack_is_empty(&resolver->primary_stack)) {
    ResolveEntry *entry = resolve_stack_peek_ptr_unchecked(&resolver->primary_stack);
    Stub *stub = compiler_get_stub(ctx->compiler, entry->stub);

    switch (stub->resolve_status) {
    case RESOLVE_STATUS_UNRESOLVED: break;
    case RESOLVE_STATUS_RESOLVING: {
      if (!entry->is_started) {
        Message_error(
          ctx->msg_sink,
          (MessageLocation){
            .kind = MessageLocation_unspecified,
            .decl_idx = entry->state.frame.decl,
          },
          string_lit("Encountered circular declaration")
        );

        clear_resolve_stack_with_error(ctx, &resolver->primary_stack);

        return False;
      }

      break;
    }
    case RESOLVE_STATUS_FULLY_RESOLVED: Panic(); // This should never happen
    case RESOLVE_STATUS_ERROR: Panic(); // This should never happen
    }

    b32 ok = try_resolve_entry(ctx, resolver, sp);

    if (!ok) {
      clear_resolve_stack_with_error(ctx, &resolver->primary_stack);
      return False;
    }
  }

  return True;
}

b32 resolve_declarations(ResolveContext *ctx, u32 decls_to_resolve_count, DeclarationKey *decls_to_resolve) {
  Resolver resolver = { 0 };

  SpecializerContext sp = {
    .perm = ctx->perm,
    .scratch = ctx->scratch,
    .msg_sink = ctx->msg_sink,
    .declarations = ctx->decls,
    .types = ctx->types,
    .values = ctx->values,
    .common = ctx->common,
    .compiler = ctx->compiler,
  };

  b32 all_ok = True;

  for (u32 i = 0; i < decls_to_resolve_count; i++) {
    DeclarationKey decl_idx = decls_to_resolve[i];
    Declaration *decl = decls_extra_get_ptr(ctx->decls, decl_idx);
    Stub *stub = compiler_get_stub(ctx->compiler, decl->data.decl.stub);

    Assert(stub->resolve_status != RESOLVE_STATUS_RESOLVING);

    if (stub->resolve_status == RESOLVE_STATUS_ERROR || stub->resolve_status == RESOLVE_STATUS_FULLY_RESOLVED) {
      continue;
    }

    ArenaSnapshot snapshot = arena_scope_begin(ctx->scratch);

    push_resolve_decl_entry(ctx, &resolver.primary_stack, decl_idx);

    b32 ok = try_resolve_primary_stack(ctx, &resolver, &sp);
    if (!ok) {
      all_ok = False;
    }

    while (!resolve_stack_is_empty(&resolver.deferred_stack)) {
      ResolveEntry entry = resolve_stack_pop(&resolver.deferred_stack);
      resolve_stack_append(&resolver.primary_stack, ctx->scratch, entry);

      b32 ok = try_resolve_primary_stack(ctx, &resolver, &sp);
      if (!ok) {
        all_ok = False;
      }
    }

    zero_struct(ResolveStack, &resolver.primary_stack);
    zero_struct(ResolveStack, &resolver.deferred_stack);

    arena_scope_end(ctx->scratch, snapshot);
  }

  return all_ok;
}

