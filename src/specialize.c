#include "specialize.h"
#include "ir.h"
#include "eval.h"

#define SEGMENTLIST_NAME BREAK_RETURN_LIST_NAME
#define SEGMENTLIST_TYPE BREAK_RETURN_LIST_TYPE
#define SEGMENTLIST_MIN_SIZE_LOG2 BREAK_RETURN_LIST_MIN_SIZE_LOG_2
#define SEGMENTLIST_SEGMENT_COUNT BREAK_RETURN_LIST_SEGMENT_COUNT
#define SEGMENTLIST_FUNCTION_PREFIX breaks
#define SEGMENTLIST_LINKAGE internal
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

#define SEGMENTLIST_NAME SCOPE_STACK_NAME
#define SEGMENTLIST_TYPE SCOPE_STACK_TYPE
#define SEGMENTLIST_MIN_SIZE_LOG2 SCOPE_STACK_MIN_SIZE_LOG_2
#define SEGMENTLIST_SEGMENT_COUNT SCOPE_STACK_SEGMENT_COUNT
#define SEGMENTLIST_FUNCTION_PREFIX scopes
#define SEGMENTLIST_LINKAGE internal
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

extern Allocator const cstd_allocator;

internal StepResult pop_finished_scopes(SpecializerContext *ctx, Specializer *state);

internal void store_inst_value(Frame *f, InstructionOffset idx, IRef val);
internal void store_inst_type(Frame *f, InstructionOffset idx, TypeKey type);

internal IRef load_inst_value(Frame *f, InstructionOffset inst) {
  return f->inst_map[inst];
}

internal TypeKey get_sref_type(SpecializerContext *ctx, Frame *f, SRef ref);
internal TypeKey get_iref_type(SpecializerContext *ctx, Specializer *sp, IRef ref);

internal b32 finalize_residual_block(SpecializerContext *ctx, IIrBuilder *builder, Frame *f, Scope *block);
internal InstructionOffset function_body_start(SIrChunk *chunk, InstructionOffset func);
internal b32 finalize_function_signature(SpecializerContext *ctx, Specializer *state, TypeKey *type_signature);

void specializer_init_function(SpecializerContext *ctx, Specializer *state, StubKey stub_key);

internal IRef resolve(SpecializerContext *ctx, Frame *f, SRef ref);

internal InstructionOffset expect_residual_at_instruction_index(Frame *f, InstructionOffset inst);
internal b32 expect_comptime_value_or_nil(SpecializerContext *ctx, Specializer *state, SRef ref, ValueKey *out);
internal b32 expect_some_comptime_value(SpecializerContext *sp, Specializer *state, SRef ref, ValueKey *out);
internal b32 _get_value_expect_type(SpecializerContext *sp, Specializer *state, ValueKey val, TypeKey *out);
internal b32 expect_some_type_value(SpecializerContext *sp, Specializer *state, SRef ref, TypeKey *out);
internal b32 expect_type_value_or_nil(SpecializerContext *sp, Specializer *state, SRef ref, TypeKey *out);
internal TypeKey type_of_val(SpecializerContext *ctx, ValueKey v);
internal ValueKey val_from_type(SpecializerContext *ctx, TypeKey t);
internal ValueKey val_from_usize(SpecializerContext *ctx, usize x);
internal TypeKey ref_typeof(SpecializerContext *ctx, Frame *f, SRef ref);
internal ValueKey create_value_pointer(SpecializerContext *sp, ValueKey idx);

// ---

internal b32 coerce(SpecializerContext *ctx, Specializer *sp, IRef ref, TypeKey dst, InstructionOffset pc, IRef *res) {
  MessageLocation loc = {
    .kind = MessageLocation_ir_instruction,
    .decl_idx = sp->frame.decl,
    .data.offset = pc,
  };

  if (iref_is_nil(ref)) {
    Message_error(ctx->msg_sink, loc, string_lit("Expected a value of type %type"), dst);
    return False;
  }

  if (iref_is_some_value(ref)) {
    Value *v = values2_get(ctx->values, iref_to_value(ref));

    ValueKey val_res;
    u32 err = eval_coerce(ctx->types, ctx->values, dst, iref_to_value(ref), &val_res);
    if (err == CoerceResult_comptime_int_value_out_of_range) {
      Message_error(ctx->msg_sink, loc, string_lit("Value of comptime_int is out of range of destination type"));
      return False;
    }

    if (err == CoerceResult_invalid_coercion_types) {
      Message_error(ctx->msg_sink, loc, string_lit("Cannot coerce value of type %type to type %type"), v->type, dst);
      return False;
    }

    *res = iref_from_value(val_res);
    return True;
  }

  TypeKey from = get_iref_type(ctx, sp, ref);
  if (key_is_nil(from)) {
    TodoMsg("coerce a runtime value whose type is unknown");
  }

  switch (Cast(CoercionResult, classify_type_coercion(ctx->types, dst, from))) {
  case Coercion_identical_types:
    *res = ref;
    return True;

  case Coercion_int_widen:
  case Coercion_bool_to_int: {
    IIrBuilder *builder = &sp->function.builder;
    InstructionOffset inst = iir_builder_add(builder, IIR_int_cast, sp->frame.chunk->sources[pc]);
    iir_builder_set_data(builder, inst, iref_to_u32(ref));
    iir_builder_set_type(builder, inst, dst);
    *res = iref_from_instruction(inst);
    return True;
  }

  case Coercion_array_to_slice: Todo();
  case Coercion_comptime: Todo(); 

  case Coercion_invalid:
    Message_error(ctx->msg_sink, loc, string_lit("Cannot coerce value of type %type to type %type"), from, dst);
    return False;
  }

  Unreachable();
}

// --- Scope

b8 scope_kind_is_comptime(ScopeKind kind) {
  switch (kind) {
  case Scope_block:
  case Scope_function:
    return False;
  case Scope_comptime:
  case Scope_signature_of_function:
    return True;
  }
}

void scope_add_break_or_return(Scope *scope, Arena *arena, InstructionOffset source) {
  breaks_append(&scope->breaks_and_returns, arena, source);
}

Scope *find_scope(ScopeStack *scopes, InstructionOffset start_of_block) {
  for (u32 i = 0; i < scopes->len; i++) {
    Scope *s = scopes_ptr_at_unchecked(scopes, i);
    if (s->start == start_of_block) {
      return s;
    }
  }

  Panic();
}

// --- Frame

void frame_init(Frame *f, Arena *arena, Declaration *decl) {
  SIrChunk *chunk = &decl->data.decl.chunk;

  *f = (Frame){
    .decl = decl->idx,
    .chunk = chunk,
    .inst_map = arena_push_array(IRef, arena, chunk->opcode_count),
    .inst_types = arena_push_array(TypeKey, arena, chunk->opcode_count),
  };

  memset(f->inst_map, 0, chunk->opcode_count * sizeof(IRef));
  memset(f->inst_types, 0, chunk->opcode_count * sizeof(TypeKey));
}

b32 frame_is_comptime(Frame *frame) { return frame->comptime_depth > 0; }

Scope *frame_push_scope(Frame *frame, Arena *arena, ScopeKind kind, InstructionOffset start, u32 count) {
  Scope *span = scopes_push(&frame->scopes, arena);

  if (scope_kind_is_comptime(kind)) {
    frame->comptime_depth++;
  }

  *span = (Scope){
    .scope_kind = kind,
    .start = start,
    .end = start + count,
    .pc = start + 1,
  };

  return span;
}

Scope frame_pop_scope(Frame *frame) {
  Scope *top = scopes_peek_ptr_unchecked(&frame->scopes);
  if (scope_kind_is_comptime(top->scope_kind)) {
    frame->comptime_depth--;
  }

  return scopes_pop(&frame->scopes);
}

Scope *frame_get_func_scope(Frame *f) {
  Scope *s = scopes_ptr_at_unchecked(&f->scopes, 0);
  Assert(s->scope_kind == Scope_function);
  return s;
}

// ---

internal void store_inst_value(Frame *f, InstructionOffset idx, IRef val) {
  f->inst_map[idx] = val;
}

internal void store_inst_type(Frame *f, InstructionOffset idx, TypeKey type) {
  f->inst_types[idx] = type;
}

internal TypeKey get_value_type(SpecializerContext *ctx, ValueKey key) {
  Value *v = values2_get(ctx->values, key);
  Type *t = types_get(ctx->types, v->type);

  if (t->kind == Type_stub) {
    Stub *stub = compiler_get_stub(ctx->compiler, Cast(ValueStubRef*, v->data)->stub);
    return stub->type_hint;
  }

  return v->type;
}

internal TypeKey get_sref_type(SpecializerContext *ctx, Frame *f, SRef ref) {
  if (sref_is_some_value(ref)) {
    Value *v = values2_get(ctx->values, sref_to_value(ref));
    return v->type;
  }

  return f->inst_types[sref_to_instruction(ref)];
}

internal TypeKey get_iref_type(SpecializerContext *ctx, Specializer *sp, IRef ref) {
  if (iref_is_some_value(ref)) {
    Value *v = values2_get(ctx->values, iref_to_value(ref));
    return v->type;
  }

  return iir_builder_get_type(&sp->function.builder, iref_to_instruction(ref));
}

internal b32 finalize_residual_block(SpecializerContext *ctx, IIrBuilder *builder, Frame *f, Scope *block) {
  iir_builder_set_data(builder, block->residual, iir_builder_offset(builder, block->residual));

  if (key_is_some(block->result_type)) {
    f->inst_types[block->start] = block->result_type;
    iir_builder_set_type(builder, block->residual, block->result_type);
    return True;
  }

  if (block->breaks_and_returns.len == 0) {
    return True;
  }

  TypeKey base_type = f->inst_types[breaks_at_unchecked(&block->breaks_and_returns, 0)];
  TypeKey type_unified = base_type;
  u32 err = UnifyResult_ok;

  for (u32 i = 1; i < block->breaks_and_returns.len; i++) {
    TypeKey t = f->inst_types[breaks_at_unchecked(&block->breaks_and_returns, i)];

    err = eval_unify(ctx->scratch, ctx->types, type_unified, t, &type_unified);

    switch (Cast(UnifyResult, err)) {
    case UnifyResult_ok:
    case UnifyResult_type_is_incomplete:
      continue;
    case UnifyResult_types_cannot_be_unified:
      Todo();
    }
  }

  if (err == UnifyResult_types_cannot_be_unified) {
    Todo();
  }

  if (err == UnifyResult_type_is_incomplete && !key_is_nil(type_unified)) {
    // partially complete type
    Todo();
  }

  f->inst_types[block->start] = type_unified;

  iir_builder_set_type(builder, block->residual, type_unified);

  return True;
}

// A value-index ref is already a comptime value; an instruction-index ref is the result recorded
// for that instruction in the current frame.
// The returned IrRef can be either a value or an instruction index that maps to a residual
// instruction.
internal IRef resolve(SpecializerContext *ctx, Frame *f, SRef ref) {
  if (sref_is_value(ref)) {
    return iref_from_value(sref_to_value(ref));
  }

  return f->inst_map[sref_to_instruction(ref)];
}

internal InstructionOffset expect_residual_at_instruction_index(Frame *f, InstructionOffset inst) {
  IRef res = f->inst_map[inst];

  Assert(iref_is_instruction(res));

  return iref_to_instruction(res);
}

internal StepResult pop_finished_scopes(SpecializerContext *ctx, Specializer *state) {
  IIrBuilder *builder = &state->function.builder;
  Frame *f = &state->frame;

  ValueKey last_value = {0};

  while (!scopes_is_empty(&f->scopes)) {
    Scope *span = scopes_peek_ptr_unchecked(&f->scopes);

    if (span->pc < span->end) {
      return (StepResult){ .kind = Step_next_step };
    }

    switch (Cast(ScopeKind, span->scope_kind)) {
    case Scope_signature_of_function: {
      TypeKey type_signature;
      b32 ok = finalize_function_signature(ctx, state, &type_signature);

      if (!ok) {
        Todo();
      }

      Stub *stub = compiler_get_stub(ctx->compiler, span->data.signature_of_function.stub);
      stub->type_hint = type_signature;

      scopes_pop(&f->scopes);

      return (StepResult){
        .kind = Step_register_function,
        .data.register_function = {
          .stub = span->data.signature_of_function.stub,
        },
      };
    } break;

    case Scope_function: {
      ResidualFunction *func;
      ResidualFunctionKey key = compiler_add_function(ctx->compiler, &func);

      iir_builder_flatten(builder, ctx->perm, &func->chunk);

      ValueResidualFunction data = { .key = key };
      last_value = values2_intern_data(ctx->values, span->data.function.function_type, &data, sizeof(data));

      scopes_pop(&f->scopes);
    } break;

    case Scope_block: {
      // Must come before finalize_residual_block, so the short-circuit br is counted as part of the block.
      if (span->logical_and) {
        SIrLogicalAndOr *logic = sir_chunk_extra(f->chunk, span->logical_and);
        AstAndSourceIndex source = f->chunk->sources[span->logical_and];

        InstructionOffset inst_br = iir_builder_add(builder, IIR_br, source);
        IIrBr *br = iir_builder_push_data(builder, inst_br, IIrBr);
        *br = (IIrBr){
          .block = span->residual,
          .value = resolve(ctx, f, sref_from_value(ctx->common->val._false)),
        };
        iir_builder_set_type(builder, inst_br, ctx->types->t_bool);

        InstructionOffset inst_condbr = expect_residual_at_instruction_index(f, span->logical_and);
        IIrCondbr *condbr = iir_builder_push_data(builder, inst_condbr, IIrCondbr);
        *condbr = (IIrCondbr){
          .cond = resolve(ctx, f, logic->lhs),
          .otherwise = inst_br,
        };
      }

      if (span->residual) {
        b32 ok = finalize_residual_block(ctx, builder, f, span);
        if (!ok) {
          Todo();
        }
      }

      if (span->condbr) {
        SIrCondbr *condbr = sir_chunk_extra(f->chunk, span->condbr);

        InstructionOffset residual_condbr =
          iref_to_instruction(resolve(ctx, f, sref_from_instruction(span->condbr)));

        IIrCondbr *data = iir_builder_push_data(builder, residual_condbr, IIrCondbr);
        *data = (IIrCondbr){
          .cond = resolve(ctx, f, condbr->cond),
          .otherwise = span->residual_otherwise,
        };
      }

      scopes_pop(&f->scopes);
    } break;

    case Scope_comptime: {
      IRef r = load_inst_value(f, span->start);
      if (iref_is_instruction(r)) {
        Todo();
      }
      last_value = iref_to_value(r);
      scopes_pop(&f->scopes);
    } break;
    }
  }

  return (StepResult){
    .kind = Step_finished,
    .data.finished = { .val = last_value },
  };
}

internal b32 expect_comptime_value_or_nil(SpecializerContext *ctx, Specializer *state, SRef ref, ValueKey *out) {
  Frame *f = &state->frame;

  IRef x = resolve(ctx, f, ref);

  if (iref_is_value(x)) {
    *out = iref_to_value(x);
    return True;
  }

  Scope *s = scopes_peek_ptr_unchecked(&f->scopes);

  Message_error(
    ctx->msg_sink,
    (MessageLocation){
      .kind = MessageLocation_ir_instruction,
      .decl_idx = state->frame.decl,
      .data.offset = s->pc,
    },
    string_lit("Value must be comptime known")
  );

  return False;
}

internal b32 expect_some_comptime_value(SpecializerContext *sp, Specializer *state, SRef ref, ValueKey *out) {
  Frame *f = &state->frame;

  ValueKey idx;
  b32 ok = expect_comptime_value_or_nil(sp, state, ref, &idx);
  if (!ok) {
    return False;
  }

  if (key_is_nil(idx)) {
    Scope *s = scopes_peek_ptr_unchecked(&f->scopes);
    Message_error(
      sp->msg_sink,
      (MessageLocation){
        .kind = MessageLocation_ir_instruction,
        .decl_idx = state->frame.decl,
        .data.offset = s->pc,
      },
      string_lit("Value may not be omitted")
    );

    return False;
  }

  *out = idx;

  return True;
}

internal b32 _get_value_expect_type(SpecializerContext *sp, Specializer *state, ValueKey val, TypeKey *out) {
  Frame *f = &state->frame;

  Value *v = values2_get(sp->values, val);

  if (!key_eq(v->type, sp->types->t_type)) {
    Scope *s = scopes_peek_ptr_unchecked(&f->scopes);
    Message_error(
      sp->msg_sink,
      (MessageLocation){
        .kind = MessageLocation_ir_instruction,
        .decl_idx = state->frame.decl,
        .data.offset = s->pc,
      },
      string_lit("Expected a type, but got something else")
    );

    return False;
  }

  *out = *Cast(TypeKey *, v->data);

  return True;
}

internal b32 expect_some_type_value(SpecializerContext *sp, Specializer *state, SRef ref, TypeKey *out) {
  ValueKey val;
  b32 ok = expect_some_comptime_value(sp, state, ref, &val);
  if (!ok) {
    return False;
  }

  TypeKey t;
  ok = _get_value_expect_type(sp, state, val, &t);
  if (!ok) {
    return False;
  }

  if (key_is_nil(t)) {
    Todo();
  }

  *out = t;

  return True;
}

internal b32 expect_type_value_or_nil(SpecializerContext *sp, Specializer *state, SRef ref, TypeKey *out) {
  ValueKey val;
  b32 ok = expect_comptime_value_or_nil(sp, state, ref, &val);
  if (!ok) {
    return False;
  }

  if (key_is_nil(val)) {
    *out = (TypeKey){0};
    return True;
  }

  return _get_value_expect_type(sp, state, val, out);
}

internal TypeKey type_of_val(SpecializerContext *ctx, ValueKey v) {
  Assert(key_is_some(v));

  Value *val = values2_get(ctx->values, v);
  return val->type;
}

internal ValueKey val_from_type(SpecializerContext *ctx, TypeKey t) {
  return values2_intern_data(ctx->values, ctx->types->t_type, &t, sizeof(t));
}

internal ValueKey val_from_usize(SpecializerContext *ctx, usize x) {
  return values2_intern_data(ctx->values, ctx->types->t_usize, &x, sizeof(x));
}

internal TypeKey ref_typeof(SpecializerContext *ctx, Frame *f, SRef ref) {
  if (sref_is_some_value(ref)) {
    Value *v = values2_get(ctx->values, sref_to_value(ref));
    return v->type;
  }

  InstructionOffset i = sref_to_instruction(ref);

  TypeKey type = f->inst_types[i];

  Assert(key_is_some(type));

  return type;
}

internal ValueKey create_value_pointer(SpecializerContext *sp, ValueKey idx) {
  TypeKey type_ptr = types_add_pointer(sp->types, values2_get(sp->values, idx)->type);

  ValuePointer data = {
    .val = idx,
    .offset = 0,
  };

  return values2_intern_data(sp->values, type_ptr, &data, sizeof(data));
}

internal b32 finalize_function_signature(SpecializerContext *ctx, Specializer *state, TypeKey *type_signature) {
  Frame *f = &state->frame;

  Scope *s = scopes_peek_ptr_unchecked(&f->scopes);
  Assert(s->scope_kind == Scope_signature_of_function);

  InstructionOffset pc = s->start;
  SIrFunc *func = sir_chunk_extra(f->chunk, pc);

  ArenaSnapshot snapshot = arena_scope_begin(ctx->scratch);

  Type *type_value = arena_push_type_function(ctx->scratch, func->param_count);
  type_value->kind = Type_function;
  type_value->data.function.param_count = func->param_count;

  InstructionOffset at = pc + 1;
  for (u32 i = 0; i < 1 + func->param_count; i++) {
    TypeKey *dst = (i == 0) ? &type_value->data.function.return_type : &type_value->data.function.param_types[i - 1];

    b32 ok = expect_type_value_or_nil(ctx, state, sref_from_instruction(at), dst);
    if (!ok) {
      arena_scope_end(ctx->scratch, snapshot);
      return False;
    }

    at += sir_chunk_block_count(f->chunk, at);
  }

  TypeKey type_func = types_intern(ctx->types, type_value);

  TypeKey type_hint;
  if (!expect_type_value_or_nil(ctx, state, func->type_hint, &type_hint)) {
    arena_scope_end(ctx->scratch, snapshot);
    return False;
  }

  u32 err = eval_unify(ctx->scratch, ctx->types, type_func, type_hint, type_signature);
  if (err != UnifyResult_ok) {
    arena_scope_end(ctx->scratch, snapshot);
    return False;
  }

  arena_scope_end(ctx->scratch, snapshot);

  return True;
}

void specializer_init_function(SpecializerContext *ctx, Specializer *state, StubKey stub_key) {
  Stub *stub = compiler_get_stub(ctx->compiler, stub_key);
  Declaration *decl = decls_extra_get_ptr(ctx->declarations, stub->decl);

  zero_struct(Specializer, state);

  state->function.builder.scratch = ctx->scratch;

  Frame *f = &state->frame;
  IIrBuilder *builder = &state->function.builder;

  frame_init(f, ctx->scratch, decl);

  SIrFunc *ir_func = sir_chunk_extra(f->chunk, stub->inst);
  Scope *scope = frame_push_scope(f, ctx->scratch, Scope_function, stub->inst, ir_func->instruction_count);

  InstructionOffset inst_func = iir_builder_add(builder, IIR_func, f->chunk->sources[stub->inst]);
  iir_builder_set_type(builder, inst_func, stub->type_hint);

  TypeFunction *type = &types_get(ctx->types, stub->type_hint)->data.function;

  scope->data.function.function_type = stub->type_hint;
  scope->data.function.return_type = type->return_type;

  InstructionOffset pc = ir_func->body;

  for (u32 i = 0; i < type->param_count; i++) {
    Assert(sir_chunk_op(f->chunk, pc + i) == SIR_param);

    InstructionOffset inst_param = iir_builder_add(builder, IIR_param, f->chunk->sources[pc + i]);
    iir_builder_set_type(builder, inst_param, type->param_types[i]);

    store_inst_value(f, pc + i, iref_from_instruction(inst_param));
    store_inst_type(f, pc + i, type->param_types[i]);
  }

  pc += type->param_count;

  scope->pc = pc;
}

internal StepResult step(SpecializerContext *ctx, Specializer *state) {
  Frame *f = &state->frame;
  IIrBuilder *builder = &state->function.builder;

  {
    StepResult res = pop_finished_scopes(ctx, state);
    if (res.kind != Step_next_step) {
      return res;
    }
  }

  Scope *s = scopes_peek_ptr_unchecked(&f->scopes);

  InstructionOffset pc = s->pc;
  SIrOpcode op = sir_chunk_op(f->chunk, pc);

  if (s->condbr && pc == Cast(SIrCondbr*, sir_chunk_extra(f->chunk, s->condbr))->otherwise) {
    s->residual_otherwise = builder->kinds.len;
  }

  switch (op) {
  case SIR_comptime_block: {
    u32 inst_count = sir_chunk_block_count(f->chunk, pc);
    frame_push_scope(f, ctx->scratch, Scope_comptime, pc, inst_count);
    s->pc += inst_count;
  } break;

  case SIR_param: {
    Todo();
    s->pc += 1;
  } break;

  case SIR_return_type: {
    TypeKey type = frame_get_func_scope(f)->data.function.return_type;
    store_inst_value(f, pc, key_is_nil(type) ? IREF_NIL : iref_from_value(val_from_type(ctx, type)));
    s->pc += 1;
  } break;

  case SIR_loop:
  case SIR_block: {
    u32 inst_count = sir_chunk_block_count(f->chunk, pc);

    TypeKey result_type = {0};
    if (op == SIR_block) {
      SIrBlock *block = sir_chunk_extra(f->chunk, pc);
      b32 ok = expect_type_value_or_nil(ctx, state, block->result_type, &result_type);
      if (!ok) {
        return (StepResult){ .kind = Step_error };
      }
    }

    Scope *scope = frame_push_scope(f, ctx->scratch, Scope_block, pc, inst_count);
    scope->result_type = result_type;

    if (!frame_is_comptime(f)) {
      InstructionOffset inst = iir_builder_add(builder, op == SIR_loop ? IIR_loop : IIR_block, f->chunk->sources[pc]);

      store_inst_value(f, pc, iref_from_instruction(inst));

      scope->residual = inst;
    }

    s->pc += inst_count;
  } break;

  case SIR_func: {
    SIrFunc *func = sir_chunk_extra(f->chunk, pc);

    DeclarationKey decl = f->decl;
    InstructionOffset inst = pc;

    StubKey stub = compiler_add_stub(ctx->compiler, decl, inst, (TypeKey){0});

    ValueStubRef stub_ref = { .stub = stub };
    ValueKey ref = values2_intern_data(ctx->values, ctx->types->t_stub, &stub_ref, sizeof(stub_ref));

    store_inst_value(f, pc, iref_from_value(ref));

    InstructionOffset body = func->body;
    Scope *scope = frame_push_scope(f, ctx->scratch, Scope_signature_of_function, pc, body - pc);
    scope->data.signature_of_function.stub = stub;

    s->pc += func->instruction_count;
  } break;

  case SIR_get_decl_value: {
    DeclarationKey decl_idx = {sir_chunk_data(f->chunk, pc)};
    Declaration *decl = decls_extra_get_ptr(ctx->declarations, decl_idx);

    switch (Cast(DeclarationKind, decl->kind)) {
    case Declaration_root: Panic(); break;
    case Declaration_mod: { Todo(); } break;
    case Declaration_primitive: {
      ValueKey ptr = create_value_pointer(ctx, decl->data.primitive);
      store_inst_value(f, pc, iref_from_value(ptr));
      s->pc += 1;
    } break;
    case Declaration_decl: {
      Stub *stub = compiler_get_stub(ctx->compiler, decl->data.decl.stub);

      switch (Cast(ResolveStatus, stub->resolve_status)) {
      case RESOLVE_STATUS_FULLY_RESOLVED: {
        ValueKey ptr = create_value_pointer(ctx, stub->val);
        store_inst_value(f, pc, iref_from_value(ptr));
        s->pc += 1;
      } break;

      case RESOLVE_STATUS_ERROR: Panic();

      case RESOLVE_STATUS_UNRESOLVED:
      case RESOLVE_STATUS_RESOLVING: {
        return (StepResult){
          .kind = Step_resolve_declaration_value,
          .data.resolve_decl = {
            .decl = decl_idx,
          },
        };
      } break;
      }
    } break;
    }
  } break;

  case SIR_as: {
    SIrAs *as = sir_chunk_extra(f->chunk, pc);
    IRef ref = resolve(ctx, f, as->val);
    IRef ref_type_to = resolve(ctx, f, as->type_dst);

    Assert(!iref_is_nil(ref));

    if (iref_is_nil(ref_type_to)) {
      if (iref_is_some_value(ref)) {
        store_inst_value(f, pc, ref);
      } else {
        Todo();
      }

      s->pc = pc + 1;
      break;
    }

    TypeKey type_dst;
    b32 ok = expect_some_type_value(ctx, state, as->type_dst, &type_dst);
    if (!ok) {
      return (StepResult){ .kind = Step_error };
    }

    f->inst_types[pc] = type_dst;

    ok = coerce(ctx, state, ref, type_dst, pc, &ref);
    if (!ok) {
      return (StepResult){ .kind = Step_error };
    }

    store_inst_value(f, pc, ref);

    s->pc = pc + 1;
  } break;

  case SIR_cast: {
    SIrAs *cast = sir_chunk_extra(f->chunk, pc);

    TypeKey type_dst;
    b32 ok = expect_some_type_value(ctx, state, cast->type_dst, &type_dst);
    if (!ok) {
      return (StepResult){ .kind = Step_error };
    }

    TypeKey type_src = ref_typeof(ctx, f, cast->val);

    if (!is_type_castable_to(ctx->types, type_dst, type_src)) {
      Message_error(
        ctx->msg_sink,
        (MessageLocation){
          .kind = MessageLocation_ir_instruction,
          .decl_idx = state->frame.decl,
          .data.offset = s->pc,
        },
        string_lit("Cannot cast value of type %type to type %type"),
        type_src,
        type_dst
      );

      return (StepResult){ .kind = Step_error };
    }

    store_inst_type(f, pc, type_dst);

    IRef ref = resolve(ctx, f, cast->val);
    Assert(!iref_is_nil(ref));

    if (key_eq(type_dst, type_src)) {
      store_inst_value(f, pc, ref);
      s->pc = pc + 1;
      break;
    }

    if (iref_is_some_value(ref)) {
      Type *t_src = types_get(ctx->types, type_src);

      TypeInteger int_src = (t_src->kind == Type_comptime_int)
        ? (TypeInteger){ .signedness = Signed, .bitwidth = sizeof(ComptimeInt) * 8 }
        : t_src->data.integer;

      TypeInteger int_dst = types_get(ctx->types, type_dst)->data.integer;

      u32 size = int_dst.bitwidth / 8;

      u64 data = 0;
      Assert(size <= sizeof(data));

      void *payload = values2_get(ctx->values, iref_to_value(ref))->data;

      u32 err = eval_cast_int(int_src, payload, int_dst, &data);
      if (err) {
        Message_error(
          ctx->msg_sink,
          (MessageLocation){
            .kind = MessageLocation_ir_instruction,
            .decl_idx = state->frame.decl,
            .data.offset = s->pc,
          },
          string_lit("Value does not fit in the destination type %type of the cast"),
          type_dst
        );

        return (StepResult){ .kind = Step_error };
      }

      ValueKey res = values2_intern_data(ctx->values, type_dst, &data, size);

      store_inst_value(f, pc, iref_from_value(res));
    } else {
      InstructionOffset inst = iir_builder_add(builder, IIR_int_cast, f->chunk->sources[pc]);
      iir_builder_set_data(builder, inst, iref_to_u32(ref));
      iir_builder_set_type(builder, inst, type_dst);

      store_inst_value(f, pc, iref_from_instruction(inst));
    }

    s->pc = pc + 1;
  } break;

  case SIR_br: {
    SIrBr *br = sir_chunk_extra(f->chunk, pc);

    TypeKey result_type = {0};
    if (sir_chunk_op(f->chunk, br->block) == SIR_block) {
      result_type = find_scope(&f->scopes, br->block)->result_type;
    }

    if (frame_is_comptime(f)) {
      ValueKey val;
      b32 ok = expect_comptime_value_or_nil(ctx, state, br->value, &val);
      if (!ok) {
        Todo();
      }

      if (key_is_some(result_type)) {
        IRef coerced;
        ok = coerce(ctx, state, iref_from_value(val), result_type, pc, &coerced);
        if (!ok) {
          return (StepResult){ .kind = Step_error };
        }

        val = iref_to_value(coerced);
      }

      if (key_is_some(val)) {
        f->inst_types[br->block] = type_of_val(ctx, val);
      }

      store_inst_value(f, br->block, iref_from_value(val));

      s->pc += 1;
      break;
    }

    if (s->scope_kind == Scope_block) {
      IRef ref = resolve(ctx, f, br->value);
      TypeKey type_br = iref_is_nil(ref) ? (TypeKey){0} : get_sref_type(ctx, f, br->value);

      if (key_is_some(result_type)) {
        b32 ok = coerce(ctx, state, ref, result_type, pc, &ref);
        if (!ok) {
          return (StepResult){ .kind = Step_error };
        }

        type_br = result_type;
      }

      InstructionOffset inst_br = iir_builder_add(builder, IIR_br, f->chunk->sources[pc]);
      IIrBr *data_br = iir_builder_push_data(builder, inst_br, IIrBr);
      *data_br = (IIrBr){
        .block = expect_residual_at_instruction_index(f, br->block),
        .value = ref,
      };

      Scope *block = find_scope(&f->scopes, br->block);
      scope_add_break_or_return(block, ctx->scratch, pc);

      store_inst_type(f, pc, type_br);
      store_inst_value(f, pc, iref_from_instruction(inst_br));

      iir_builder_set_type(builder, inst_br, type_br);

      s->pc += 1;
      break;
    }

    Unreachable();
  } break;

  case SIR_type: {
    SIrType *type = sir_chunk_extra(f->chunk, pc);

    TypeKey t;
    switch (Cast(TypeKind, type->kind)) {
    case Type_function: {
      Assert(type->arg_count != 0);

      u32 param_count = type->arg_count - 1;

      TypeKey return_type;
      b32 ok = expect_type_value_or_nil(ctx, state, type->args[0], &return_type);
      Assert(ok);

      ArenaSnapshot snapshot = arena_scope_begin(ctx->scratch);

      Type *func = arena_push_type_function(ctx->scratch, param_count);
      func->kind = Type_function;
      func->data.function.return_type = return_type;
      func->data.function.param_count = param_count;

      for (u32 i = 0; i < param_count; i++) {
        ok = expect_type_value_or_nil(ctx, state, type->args[1 + i], &func->data.function.param_types[i]);
        Assert(ok);
      }

      t = types_intern(ctx->types, func);

      arena_scope_end(ctx->scratch, snapshot);
    } break;

    case Type_slice: {
      Assert(type->arg_count == 1);

      TypeKey base_type;
      b32 ok = expect_type_value_or_nil(ctx, state, type->args[0], &base_type);
      if (!ok) {
        Todo();
      }

      t = types_intern(
        ctx->types,
        &(Type){
          .kind = Type_slice,
          .data.slice = { .base_type = base_type },
        }
      );
    } break;

    case Type_array: Todo();

    default:
      Panic();
    }

    ValueKey v = val_from_type(ctx, t);
    store_inst_value(f, pc, iref_from_value(v));
    s->pc += 1;
  } break;

  case SIR_unify: {
    SIrUnify *unify = sir_chunk_extra(f->chunk, pc);

    b32 ok = True;

    TypeKey type_lhs;
    ok = expect_type_value_or_nil(ctx, state, unify->type_lhs, &type_lhs);
    if (!ok) {
      return (StepResult){ .kind = Step_error };
    }

    TypeKey type_rhs;
    ok = expect_type_value_or_nil(ctx, state, unify->type_rhs, &type_rhs);
    if (!ok) {
      return (StepResult){ .kind = Step_error };
    }

    TypeKey type_unified;
    u32 err = eval_unify(ctx->scratch, ctx->types, type_lhs, type_rhs, &type_unified);
    if (err) {
      Message_error(
        ctx->msg_sink,
        (MessageLocation){
          .kind = MessageLocation_ir_instruction,
          .decl_idx = state->frame.decl,
          .data.offset = s->pc,
        },
        string_lit("Unable to unify types"));
      return (StepResult){ .kind = Step_error };
    }

    ValueKey v = val_from_type(ctx, type_unified);
    store_inst_value(f, pc, iref_from_value(v));
    s->pc += 1;
  } break;

  case SIR_ret: {
    SRef ref_val = (SRef){sir_chunk_data(f->chunk, pc)};
    IRef val = resolve(ctx, f, ref_val);

    Scope *func_scope = frame_get_func_scope(f);
    TypeKey return_type = func_scope->data.function.return_type;

    if (key_is_some(return_type)) {
      b32 ok = coerce(ctx, state, val, return_type, pc, &val);
      if (!ok) {
        return (StepResult){ .kind = Step_error };
      }
    }

    InstructionOffset inst = iir_builder_add(builder, IIR_ret, f->chunk->sources[pc]);

    TypeKey type_ret;
    if (key_is_some(return_type)) {
      type_ret = return_type;
    } else if (iref_is_some_value(val)) {
      type_ret = type_of_val(ctx, iref_to_value(val));
    } else if (iref_is_nil(val)) {
      type_ret = (TypeKey){0};
    } else {
      type_ret = f->inst_types[sref_to_instruction(ref_val)];
    }

    f->inst_types[pc] = type_ret;

    iir_builder_set_data(builder, inst, iref_to_u32(val));
    iir_builder_set_type(builder, inst, type_ret);

    scope_add_break_or_return(func_scope, ctx->scratch, pc);

    s->pc += 1;
  } break;

  case SIR_condbr: {
    SIrCondbr *condbr = sir_chunk_extra(f->chunk, pc);

    IRef cond = resolve(ctx, f, condbr->cond);

    if (iref_is_some_value(cond)) {
      if (key_eq(iref_to_value(cond), (ValueKey){VAL_BOOL_TRUE})) {
        s->pc += 1;
      } else {
        s->pc = condbr->otherwise;
      }
    } else {
      InstructionOffset inst_condbr = iir_builder_add(builder, IIR_condbr, f->chunk->sources[pc]);

      store_inst_value(f, pc, iref_from_instruction(inst_condbr));

      s->condbr = pc;
      s->pc += 1;
    }
  } break;

  case SIR_call: {
    SIrCall *call = sir_chunk_extra(f->chunk, pc);

    InstructionOffset inst_call = iir_builder_add(builder, IIR_call, f->chunk->sources[pc]);

    IIrCall *data_call = iir_builder_push_data_raw(
      builder,
      inst_call,
      sizeof(IIrCall) + call->arg_count * sizeof(IRef),
      Align_of(IIrCall)
    );

    IRef func = resolve(ctx, f, call->func);

    *data_call = (IIrCall){
      .func_ptr = func,
      .arg_count = call->arg_count,
    };

    for (u32 i = 0; i < call->arg_count; i++) {
      data_call->args[i] = resolve(ctx, f, call->args[i]);
    }

    if (iref_is_some_value(func)) {
      TypeKey t = get_value_type(ctx, iref_to_value(func));
      Type *func_type = types_get(ctx->types, t);

      if (func_type->kind != Type_function) {
        Todo();
      }

      store_inst_type(f, pc, func_type->data.function.return_type);
      iir_builder_set_type(builder, inst_call, func_type->data.function.return_type);
    } else {
      Todo();
    }

    store_inst_value(f, pc, iref_from_instruction(inst_call));

    s->pc += 1;
  } break;

  case SIR_builtin_len: {
    SRef ref = (SRef){sir_chunk_data(f->chunk, pc)};
    Type *t = types_get(ctx->types, ref_typeof(ctx, f, ref));

    store_inst_type(f, pc, ctx->types->t_usize);

    // An array carries its length in its type, so #len is comptime known even when the array
    // itself is not.
    if (t->kind == Type_array) {
      store_inst_value(f, pc, iref_from_value(val_from_usize(ctx, t->data.array.size)));
      s->pc += 1;
      break;
    }

    if (t->kind != Type_slice) {
      Message_error(
        ctx->msg_sink,
        (MessageLocation){
          .kind = MessageLocation_ir_instruction,
          .decl_idx = state->frame.decl,
          .data.offset = s->pc,
        },
        string_lit("#len expects an array or a slice, but got a value of type %type"),
        ref_typeof(ctx, f, ref)
      );

      return (StepResult){ .kind = Step_error };
    }

    IRef val = resolve(ctx, f, ref);

    if (iref_is_some_value(val)) {
      Value *v = values2_get(ctx->values, iref_to_value(val));
      store_inst_value(f, pc, iref_from_value(val_from_usize(ctx, Cast(ValueSlice *, v->data)->len)));
      s->pc += 1;
      break;
    }

    InstructionOffset inst = iir_builder_add(builder, IIR_builtin_len, f->chunk->sources[pc]);
    iir_builder_set_type(builder, inst, ctx->types->t_usize);
    iir_builder_set_data(builder, inst, iref_to_u32(val));

    store_inst_value(f, pc, iref_from_instruction(inst));

    s->pc += 1;
  } break;

  case SIR_builtin_debug: {
    SRef ref = (SRef){sir_chunk_data(f->chunk, pc)};

    TypeKey type;
    if (sref_is_some_value(ref)) {
      type = type_of_val(ctx, sref_to_value(ref));
    } else {
      type = f->inst_types[sref_to_instruction(ref)];
    }

    f->inst_types[pc] = type;

    IRef val = resolve(ctx, f, ref);

    InstructionOffset inst = iir_builder_add(builder, IIR_builtin_debug, f->chunk->sources[pc]);
    iir_builder_set_type(builder, inst, type);
    iir_builder_set_data(builder, inst, iref_to_u32(val));

    store_inst_value(f, pc, iref_from_instruction(inst));

    s->pc += 1;
  } break;

  case SIR_load: {
    SRef ref = (SRef){sir_chunk_data(f->chunk, pc)};
    IRef val = resolve(ctx, f, ref);

    // TODO make sure it is a pionter

    if (iref_is_some_value(val)) {
      Value *v = values2_get(ctx->values, iref_to_value(val));
      ValuePointer *p = Cast(ValuePointer *, v->data);
      Assert(p->offset == 0);

      store_inst_value(f, pc, iref_from_value(p->val));

      TypeKey type_idx = type_of_val(ctx, iref_to_value(val));
      Type *t = types_get(ctx->types, type_idx);
      Assert(t->kind == Type_pointer);

      f->inst_types[pc] = t->data.pointer.base_type;
    } else {
      InstructionOffset inst = iir_builder_add(builder, IIR_load, f->chunk->sources[pc]);
      TypeKey type = iir_builder_get_type(builder, iref_to_instruction(val));
      iir_builder_set_type(builder, inst, type);
      iir_builder_set_data(builder, inst, iref_to_u32(val));

      store_inst_value(f, pc, iref_from_instruction(inst));

      TypeKey type_idx = ref_typeof(ctx, f, ref);
      Type *t = types_get(ctx->types, type_idx);
      Assert(t->kind == Type_pointer);

      f->inst_types[pc] = t->data.pointer.base_type;
    }

    s->pc += 1;
  } break;

  case SIR_store: {
    SIrStore *store = sir_chunk_extra(f->chunk, pc);

    IRef dst = resolve(ctx, f, store->dst);

    if (iref_is_some_value(dst)) {
      Todo();
    } else {
      IRef val = resolve(ctx, f, store->value);

      TypeKey type_dst;
      {
        TypeKey type_dst_ptr = get_sref_type(ctx, f, store->dst);
        Type *t = types_get(ctx->types, type_dst_ptr);
        Assert(t->kind == Type_pointer);
        type_dst = t->data.pointer.base_type;
      }

      b32 ok = coerce(ctx, state, val, type_dst, pc, &val);
      if (!ok) {
        return (StepResult){ .kind = Step_error };
      }

      InstructionOffset inst = iir_builder_add(builder, IIR_store, f->chunk->sources[pc]);
      IIrStore *data = iir_builder_push_data(builder, inst, IIrStore);

      *data = (IIrStore){
        .ptr = dst,
        .value = val,
      };

      iir_builder_set_type(builder, inst, type_dst);
    }

    s->pc += 1;
  } break;

  case SIR_alloc: {
    TypeKey type;
    b32 ok = expect_some_type_value(ctx, state, (SRef){sir_chunk_data(f->chunk, pc)}, &type);
    if (!ok) {
      Todo();
    }

    // TODO type may not be comptime

    InstructionOffset inst = iir_builder_add(builder, IIR_alloc, f->chunk->sources[pc]);

    iir_builder_set_type(builder, inst, type);

    store_inst_value(f, pc, iref_from_instruction(inst));

    TypeKey ptr_type = types_add_pointer(ctx->types, type);
    f->inst_types[pc] = ptr_type;

    s->pc += 1;
  } break;

  case SIR_comptime_alloc: {
    TypeKey type;
    b32 ok = expect_some_type_value(ctx, state, (SRef){sir_chunk_data(f->chunk, pc)}, &type);
    Assert(ok);

    ValueKey idx_alloc;
    {
      ArenaSnapshot snapshot = arena_scope_begin(ctx->scratch);

      TypeSizeInfo size_info = types_size_info_by_key(ctx->types, type);
      void *data = arena_push(ctx->scratch, size_info.size, size_info.align);
      memset(data, 0, size_info.size);
      idx_alloc = values2_intern_data(ctx->values, type, data, size_info.size);

      arena_scope_end(ctx->scratch, snapshot);
    }

    ValueKey idx_p = create_value_pointer(ctx, idx_alloc);

    store_inst_value(f, pc, iref_from_value(idx_p));

    s->pc += 1;
  } break;

  case SIR_base_type: {
    SRef ref = (SRef){sir_chunk_data(f->chunk, pc)};

    TypeKey type;
    b32 ok = expect_some_type_value(ctx, state, ref, &type);
    if (!ok) {
      Todo();
    }

    Type *t = types_get(ctx->types, type);

    TypeKey base_type;
    // clang-format off
    switch (t->kind) {
    case Type_pointer: base_type = t->data.pointer.base_type; break;
    case Type_array: base_type = t->data.array.base_type; break;
    case Type_slice: base_type = t->data.slice.base_type; break;
    default: {
      Message_error(
        ctx->msg_sink,
        (MessageLocation){
          .kind = MessageLocation_ir_instruction,
          .decl_idx = state->frame.decl,
          .data.offset = s->pc,
        },
        string_lit("Tried to get base type of type %type that is not a pointer, array or slice. This is likely a compiler bug"),
        type
      );
      return (StepResult){ .kind = Step_error };
    } break;
    }
    // clang-format on

    ValueKey v = val_from_type(ctx, base_type);

    store_inst_value(f, pc, iref_from_value(v));

    s->pc += 1;
  } break;

  case SIR_typeof: {
    SRef ref = (SRef){sir_chunk_data(f->chunk, pc)};
    IRef val = resolve(ctx, f, ref);

    if (iref_is_some_value(val)) {
      TypeKey t = type_of_val(ctx, iref_to_value(val));
      ValueKey v = val_from_type(ctx, t);
      store_inst_value(f, pc, iref_from_value(v));
    } else {
      InstructionOffset i = sref_to_instruction(ref);
      TypeKey t = f->inst_types[i];
      ValueKey v = val_from_type(ctx, t);
      store_inst_value(f, pc, iref_from_value(v));
    }

    s->pc += 1;
  } break;

  case SIR_typeof_param: {
    SIrTypeofParam *typeof_param = sir_chunk_extra(f->chunk, pc);

    TypeKey type;
    b32 ok = expect_some_type_value(ctx, state, typeof_param->function_type, &type);
    if (!ok) {
      return (StepResult){ .kind = Step_error };
    }

    Type *t = types_get(ctx->types, type);

    if (t->kind != Type_function) {
      Message_error(
        ctx->msg_sink,
        (MessageLocation){
          .kind = MessageLocation_ir_instruction,
          .decl_idx = state->frame.decl,
          .data.offset = s->pc,
        },
        string_lit("Cannot call a value of type %type"),
        type
      );

      return (StepResult){ .kind = Step_error };
    }

    if (typeof_param->param_index >= t->data.function.param_count) {
      Message_error(
        ctx->msg_sink,
        (MessageLocation){
          .kind = MessageLocation_ir_instruction,
          .decl_idx = state->frame.decl,
          .data.offset = s->pc,
        },
        string_lit("Too many arguments for function of type %type"),
        type
      );

      return (StepResult){ .kind = Step_error };
    }

    ValueKey v = val_from_type(ctx, t->data.function.param_types[typeof_param->param_index]);
    store_inst_value(f, pc, iref_from_value(v));

    s->pc += 1;
  } break;

  case SIR_bitshift_left:
  case SIR_bitshift_right: {
    Todo();
  } break;

  case SIR_logical_or:
  case SIR_logical_and: {
    SIrLogicalAndOr *data = sir_chunk_extra(f->chunk, pc);

    TypeKey type_lhs = get_sref_type(ctx, f, data->lhs);

    if (!key_eq(type_lhs, ctx->types->t_bool)) {
      Todo();
    }

    IRef lhs = resolve(ctx, f, data->lhs);
    if (iref_is_some_value(lhs)) {
      ValueKey short_circuit_bool = (op == SIR_logical_or) ? (ValueKey){VAL_BOOL_TRUE} : (ValueKey){VAL_BOOL_FALSE};
      if (key_eq(iref_to_value(lhs), short_circuit_bool)) {
        store_inst_value(f, data->block, lhs);

        u32 inst_count = sir_chunk_block_count(f->chunk, data->block);
        s->pc = data->block + inst_count;
        break;
      }

      s->pc += 1;
      break;
    }

    Assert(s->start == data->block);

    InstructionOffset inst_condbr = iir_builder_add(builder, IIR_condbr, f->chunk->sources[pc]);
    store_inst_value(f, pc, iref_from_instruction(inst_condbr));

    if (op == SIR_logical_or) {
      InstructionOffset inst_br = iir_builder_add(builder, IIR_br, f->chunk->sources[pc]);
      IIrBr *br = iir_builder_push_data(builder, inst_br, IIrBr);
      *br = (IIrBr){
        .block = s->residual,
        .value = resolve(ctx, f, sref_from_value(ctx->common->val._true)),
      };
      iir_builder_set_type(builder, inst_br, ctx->types->t_bool);

      IIrCondbr *condbr = iir_builder_push_data(builder, inst_condbr, IIrCondbr);
      *condbr = (IIrCondbr){
        .cond = lhs,
        .otherwise = inst_br + 1,
      };
    } else {
      s->logical_and = pc;
    }

    s->pc += 1;
  } break;

  case SIR_bit_and:
  case SIR_bit_or:
  case SIR_bit_xor:
  case SIR_mul:
  case SIR_div:
  case SIR_mod:
  case SIR_sub:
  case SIR_add: {
    SIrBinary *bin = sir_chunk_extra(f->chunk, pc);

    TypeKey type_lhs = get_sref_type(ctx, f, bin->lhs);
    TypeKey type_rhs = get_sref_type(ctx, f, bin->rhs);

    Assert(!key_is_nil(type_lhs));
    Assert(!key_is_nil(type_rhs));

    TypeKey type_unified;
    u32 err = eval_unify(ctx->scratch, ctx->types, type_lhs, type_rhs, &type_unified);
    if (err != UnifyResult_ok) {
      Todo();
    }

    Type *t = types_get(ctx->types, type_unified);

    b32 ok = check_can_type_add(t);
    if (!ok) {
      Message_error(
        ctx->msg_sink,
        (MessageLocation){
          .kind = MessageLocation_ir_instruction,
          .decl_idx = state->frame.decl,
          .data.offset = s->pc,
        },
        string_lit("Type %type does not support operator"),
        type_lhs
      );

      return (StepResult){ .kind = Step_error };
    }

    store_inst_type(f, pc, type_unified);

    IRef lhs = resolve(ctx, f, bin->lhs);
    if (!key_eq(type_lhs, type_unified)) {
      ok = coerce(ctx, state, lhs, type_unified, pc, &lhs);
      if (!ok) {
        return (StepResult){ .kind = Step_error };
      }
    }

    IRef rhs = resolve(ctx, f, bin->rhs);
    if (!key_eq(type_rhs, type_unified)) {
      ok = coerce(ctx, state, rhs, type_unified, pc, &rhs);
      if (!ok) {
        return (StepResult){ .kind = Step_error };
      }
    }

    if (iref_is_some_value(lhs) && iref_is_some_value(rhs)) {
      Todo();
      break;
    }

    u8 iir_op;
    // clang-format off
    switch (op) {
    case SIR_mul: iir_op = IIR_int_mul; break;
    case SIR_div: iir_op = IIR_int_div; break;
    case SIR_mod: iir_op = IIR_int_mod; break;
    case SIR_sub: iir_op = IIR_int_sub; break;
    case SIR_add: iir_op = IIR_int_add; break;
    case SIR_bit_and: iir_op = IIR_bit_and; break;
    case SIR_bit_or: iir_op = IIR_bit_or; break;
    case SIR_bit_xor: iir_op = IIR_bit_xor; break;
    default: Unreachable();
    }
    // clang-format on

    InstructionOffset inst = iir_builder_add(builder, iir_op, f->chunk->sources[pc]);
    IIrBinary *data = iir_builder_push_data(builder, inst, IIrBinary);
    *data = (IIrBinary){ .lhs = lhs, .rhs = rhs, };

    iir_builder_set_type(builder, inst, type_unified);

    store_inst_value(f, pc, iref_from_instruction(inst));

    s->pc += 1;
  } break;


  case SIR_repeat: {
    InstructionOffset loop = sir_chunk_data(f->chunk, pc);

    if (frame_is_comptime(f)) {
      Todo();
    }

    InstructionOffset inst = iir_builder_add(builder, IIR_repeat, f->chunk->sources[pc]);
    iir_builder_set_data(builder, inst, expect_residual_at_instruction_index(f, loop));

    store_inst_value(f, pc, iref_from_instruction(inst));

    s->pc += 1;
  } break;

  case SIR_cmp_eq:
  case SIR_cmp_ne:
  case SIR_cmp_gt:
  case SIR_cmp_ge:
  case SIR_cmp_lt:
  case SIR_cmp_le: {
    SIrBinary *bin = sir_chunk_extra(f->chunk, pc);

    TypeKey type_lhs = get_sref_type(ctx, f, bin->lhs);
    TypeKey type_rhs = get_sref_type(ctx, f, bin->rhs);

    Assert(!key_is_nil(type_lhs));
    Assert(!key_is_nil(type_rhs));

    TypeKey type_unified;
    u32 err = eval_unify(ctx->scratch, ctx->types, type_lhs, type_rhs, &type_unified);
    if (err != UnifyResult_ok) {
      Todo();
    }

    Type *t = types_get(ctx->types, type_unified);
    b32 ok = check_can_type_cmp(t);
    if (!ok) {
      Todo();
    }

    store_inst_type(f, pc, ctx->types->t_bool);

    IRef lhs = resolve(ctx, f, bin->lhs);
    if (!key_eq(type_lhs, type_unified)) {
      ok = coerce(ctx, state, lhs, type_unified, pc, &lhs);
      if (!ok) {
        return (StepResult){ .kind = Step_error };
      }
    }

    IRef rhs = resolve(ctx, f, bin->rhs);
    if (!key_eq(type_rhs, type_unified)) {
      ok = coerce(ctx, state, rhs, type_unified, pc, &rhs);
      if (!ok) {
        return (StepResult){ .kind = Step_error };
      }
    }

    if (iref_is_some_value(lhs) && iref_is_some_value(rhs)) {
      Todo();
      break;
    }

    u8 iir_op;
    // clang-format off
    switch (op) {
    case SIR_cmp_eq: iir_op = IIR_int_cmp_eq; break;
    case SIR_cmp_ne: iir_op = IIR_int_cmp_ne; break;
    case SIR_cmp_gt: iir_op = IIR_int_cmp_gt; break;
    case SIR_cmp_ge: iir_op = IIR_int_cmp_ge; break;
    case SIR_cmp_lt: iir_op = IIR_int_cmp_lt; break;
    case SIR_cmp_le: iir_op = IIR_int_cmp_le; break;
    default: Unreachable();
    }
    // clang-format on

    InstructionOffset inst = iir_builder_add(builder, iir_op, f->chunk->sources[pc]);
    IIrBinary *data = iir_builder_push_data(builder, inst, IIrBinary);
    *data = (IIrBinary){
      .lhs = lhs,
      .rhs = rhs,
    };

    iir_builder_set_type(builder, inst, ctx->types->t_bool);

    store_inst_value(f, pc, iref_from_instruction(inst));

    s->pc += 1;
  } break;

  case SIR_index: {
    SIrBinary *bin = sir_chunk_extra(f->chunk, pc);

    TypeKey type_indexable = ref_typeof(ctx, f, bin->lhs);
    Type *t = types_get(ctx->types, type_indexable);

    if (t->kind != Type_array && t->kind != Type_slice) {
      Message_error(
        ctx->msg_sink,
        (MessageLocation){
          .kind = MessageLocation_ir_instruction,
          .decl_idx = state->frame.decl,
          .data.offset = s->pc,
        },
        string_lit("Cannot index a value of type %type, expected an array or a slice"),
        type_indexable
      );

      return (StepResult){ .kind = Step_error };
    }

    b32 is_array = t->kind == Type_array;
    TypeKey base_type = is_array ? t->data.array.base_type : t->data.slice.base_type;

    store_inst_type(f, pc, base_type);

    IRef indexable = resolve(ctx, f, bin->lhs);
    IRef index = resolve(ctx, f, bin->rhs);

    if (iref_is_some_value(indexable) && iref_is_some_value(index)) {
      Value *v_indexable = values2_get(ctx->values, iref_to_value(indexable));

      u8 *base;
      u64 len;
      if (is_array) {
        base = v_indexable->data;
        len = t->data.array.size;
      } else {
        ValueSlice *slice = Cast(ValueSlice *, v_indexable->data);
        base = Cast(u8 *, values2_get(ctx->values, slice->ptr.val)->data) + slice->ptr.offset;
        len = slice->len;
      }

      // The index is coerced to a usize by codegen, so it is always 8 bytes wide here.
      u64 at = read_int_zero_extend(64, values2_get(ctx->values, iref_to_value(index))->data);

      if (at >= len) {
        Message_error(
          ctx->msg_sink,
          (MessageLocation){
            .kind = MessageLocation_ir_instruction,
            .decl_idx = state->frame.decl,
            .data.offset = s->pc,
          },
          string_lit("Index is out of bounds")
        );

        return (StepResult){ .kind = Step_error };
      }

      TypeSizeInfo size_info = types_size_info_by_key(ctx->types, base_type);

      ValueKey res = values2_intern_data(ctx->values, base_type, base + at * size_info.stride, size_info.size);

      store_inst_value(f, pc, iref_from_value(res));

      s->pc += 1;
      break;
    }

    InstructionOffset inst = iir_builder_add(builder, IIR_index, f->chunk->sources[pc]);
    IIrBinary *data = iir_builder_push_data(builder, inst, IIrBinary);
    *data = (IIrBinary){
      .lhs = indexable,
      .rhs = index,
    };

    iir_builder_set_type(builder, inst, base_type);

    store_inst_value(f, pc, iref_from_instruction(inst));

    s->pc += 1;
  } break;

  case SIR_negate: {
    SRef ref = (SRef){sir_chunk_data(f->chunk, pc)};

    TypeKey type = ref_typeof(ctx, f, ref);
    Type *t = types_get(ctx->types, type);

    if (!check_can_type_add(t)) {
      Message_error(
        ctx->msg_sink,
        (MessageLocation){
          .kind = MessageLocation_ir_instruction,
          .decl_idx = state->frame.decl,
          .data.offset = s->pc,
        },
        string_lit("Type %type does not support negation"),
        type
      );

      return (StepResult){ .kind = Step_error };
    }

    // A comptime_int carries no TypeInteger of its own; it is stored as a ComptimeInt.
    TypeInteger int_type = (t->kind == Type_comptime_int)
      ? (TypeInteger){ .signedness = Signed, .bitwidth = sizeof(ComptimeInt) * 8 }
      : t->data.integer;

    u32 size = int_type.bitwidth / 8;

    store_inst_type(f, pc, type);

    IRef val = resolve(ctx, f, ref);

    // Negation is 0 - x, which gets subtraction's overflow checking for free. Negating the
    // most negative value of a signed type overflows, as does negating any non-zero unsigned.
    if (iref_is_some_value(val)) {
      u8 zero[sizeof(u64)] = {0};
      Assert(size <= sizeof(zero));

      u64 data = 0;

      b32 ok = eval_int_sub_safe(
        int_type,
        zero,
        values2_get(ctx->values, iref_to_value(val))->data,
        &data
      );
      if (!ok) {
        Message_error(
          ctx->msg_sink,
          (MessageLocation){
            .kind = MessageLocation_ir_instruction,
            .decl_idx = state->frame.decl,
            .data.offset = s->pc,
          },
          string_lit("Negating this value overflows its type %type"),
          type
        );

        return (StepResult){ .kind = Step_error };
      }

      ValueKey res = values2_intern_data(ctx->values, type, &data, size);

      store_inst_value(f, pc, iref_from_value(res));

      s->pc += 1;
      break;
    }

    u64 data_zero = 0;
    Assert(size <= sizeof(data_zero));
    ValueKey zero = values2_intern_data(ctx->values, type, &data_zero, size);

    InstructionOffset inst = iir_builder_add(builder, IIR_int_sub, f->chunk->sources[pc]);
    IIrBinary *data = iir_builder_push_data(builder, inst, IIrBinary);
    *data = (IIrBinary){
      .lhs = iref_from_value(zero),
      .rhs = val,
    };

    iir_builder_set_type(builder, inst, type);

    store_inst_value(f, pc, iref_from_instruction(inst));

    s->pc += 1;
  } break;

  case SIR_not: {
    SRef val = (SRef){sir_chunk_data(f->chunk, pc)};
    TypeKey type = get_sref_type(ctx, f, val);

    if (!key_eq(type, ctx->types->t_bool)) {
      Todo();
    }

    store_inst_type(f, pc, type);

    IRef res = resolve(ctx, f, val);

    if (iref_is_some_value(res)) {
      b32 b = *Cast(u8 *, values2_get(ctx->values, iref_to_value(res))->data);
      store_inst_value(f, pc, iref_from_value(b ? ctx->common->val._false : ctx->common->val._true));
    } else {
      InstructionOffset inst = iir_builder_add(builder, IIR_bit_not, f->chunk->sources[pc]);
      iir_builder_set_data(builder, inst, iref_to_u32(res));
      iir_builder_set_type(builder, inst, type);

      store_inst_value(f, pc, iref_from_instruction(inst));
    }

    s->pc += 1;
  } break;
  }

  return (StepResult){ .kind = Step_next_step };
}

StepResult run_toplevel_block(SpecializerContext *ctx, Specializer *state) {
  while (True) {
    StepResult res = step(ctx, state);

    switch (res.kind) {
    case Step_next_step: continue;
    case Step_finished:
    case Step_error:
    case Step_resolve_declaration_value:
    case Step_register_function:
      return res;
    }

    Unreachable();
  }

  Unreachable();
}

