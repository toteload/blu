#include "specialize.h"
#include "ir.h"
#include "eval.h"

extern Allocator const cstd_allocator;

internal TypeIndex ref_typeof(Specializer *in, CallFrame *f, SRef ref);

ScopeSpan *find_scope(ScopeSpan *spans, u32 count, InstructionIndex start_of_block) {
  for (u32 i = 0; i < count; i++) {
    if (spans[i].start == start_of_block) {
      return spans + i;
    }
  }

  Unreachable();
}

void scope_add_break_or_return(ScopeSpan *scope, InstructionIndex source) {
  if (scope->breaks_and_returns.len == MAX_BREAKS_AND_RETURNS) {
    Todo();
  }

  u32 i = scope->breaks_and_returns.len++;
  scope->breaks_and_returns.sources[i] = source;
}

internal IIrBuilder *push_ir_builder(Specializer *in) {
  IIrBuilder *builder = stack_push_ptr(&in->builders);
  *builder = (IIrBuilder){.scratch = in->scratch};
  return builder;
}

internal void pop_ir_builder(Specializer *in) { stack_pop(&in->builders); }

internal IIrBuilder *get_builder(Specializer *in) { return stack_peek_ptr(&in->builders); }

internal always_inline void store_inst_value(CallFrame *f, InstructionIndex idx, IRef val) {
  f->inst_map[idx] = val;
}

void runstate_init(RunState *state, Arena *arena) {
  state->requested_resolution = False;
  stack_init(
    &state->call_stack,
    arena_push_array(CallFrame, arena, MAX_CALL_DEPTH),
    MAX_CALL_DEPTH
  );
}

CallFrame *top_frame(RunState *state) { return stack_peek_ptr(&state->call_stack); }

ScopeSpan *get_func_scope(CallFrame *frame) {
  for (u32 i = frame->scopes.len; i-- > 0;) {
    ScopeSpan *s = &frame->scopes.data[i];
    if (s->scope_kind == Scope_func) {
      return s;
    }
  }

  Unreachable();
}

ScopeSpan *push_scope(CallFrame *frame) { return stack_push_ptr(&frame->scopes); }

internal b32 end_residual_block(IIrBuilder *builder, CallFrame *f, ScopeSpan *block) {
  iir_builder_set_data(builder, block->residual, iir_builder_offset(builder, block->residual));

  if (block->breaks_and_returns.len == 0) {
    return True;
  }

  TypeIndex type = f->inst_types[block->breaks_and_returns.sources[0]];

  for (u32 i = 1; i < block->breaks_and_returns.len; i++) {
    TypeIndex t = f->inst_types[block->breaks_and_returns.sources[i]];
    if (t != type) {
      Todo();
    }
  }

  f->inst_types[block->start] = type;

  iir_builder_set_type(builder, block->residual, type);

  return True;
}

internal b32 finalize_function(Specializer *in, CallFrame *f, ScopeSpan *func) {
  IIrBuilder *builder = get_builder(in);

  while (True) {
    ScopeSpan *s = stack_peek_ptr(&f->scopes);
    if (s == func) {
      break;
    }

    if (s->scope_kind == Scope_block) {
      b32 ok = end_residual_block(builder, f, s);
      if (!ok) {
        return False;
      }
    }

    stack_pop(&f->scopes);
  }

  Assert(func->breaks_and_returns.len > 0);

  TypeIndex return_type = f->inst_types[func->breaks_and_returns.sources[0]];

  for (u32 i = 1; i < func->breaks_and_returns.len; i++) {
    Todo();
  }

  return True;
}

CallFrame *frame_push(RunState *state, Arena *arena, Declaration *decl) {
  SIrChunk *chunk = &decl->data.decl.chunk;

  CallFrame *f = stack_push_ptr(&state->call_stack);
  *f = (CallFrame){
    .decl_idx = decl->idx,
    .chunk = chunk,
    .inst_map = arena_push_array(IRef, arena, chunk->opcode_count),
    .inst_types = arena_push_array(TypeIndex, arena, chunk->opcode_count),
    .snapshot = arena_scope_begin(arena),
  };

  memset(f->inst_map, 0, chunk->opcode_count * sizeof(IRef));
  memset(f->inst_types, 0, chunk->opcode_count * sizeof(TypeIndex));

  stack_init(&f->scopes, arena_push_array(ScopeSpan, arena, MAX_SCOPE_DEPTH), MAX_SCOPE_DEPTH);

  stack_push(
    &f->scopes,
    ((ScopeSpan){
      .scope_kind = Scope_chunk,
      .start = 0,
      .end = f->chunk->opcode_count,
      .pc = 0,
    })
  );

  return f;
}

void frame_pop(RunState *state, Arena *arena, ValueStore *values) {
  CallFrame f = stack_pop(&state->call_stack);

  ScopeSpan span = f.scopes.data[0];
  for (u32 i = span.start; i < span.end; i++) {
    if (iref_is_some_value(f.inst_map[i])) {
      values_dealloc(values, iref_to_value(f.inst_map[i]));
    }
  }

  arena_scope_end(arena, f.snapshot);
}

internal void dealloc_scope_values(Specializer *in, CallFrame *f, ScopeSpan span) {
  // Do not dealloc the value stored at the block address
  for (u32 i = span.start + 1; i < span.end; i++) {
    if (iref_is_some_value(f->inst_map[i])) {
      values_dealloc(in->values, iref_to_value(f->inst_map[i]));
      f->inst_map[i] = (IRef){0};
    }
  }
}

internal void pop_scopes_to(Specializer *in, CallFrame *f, InstructionIndex idx) {
  while (True) {
    ScopeSpan span = stack_pop(&f->scopes);

    if (span.start == idx) {
      dealloc_scope_values(in, f, span);
      return;
    }
  }

  Unreachable();
}

// A value-index ref is already a comptime value; an instruction-index ref is the result recorded
// for that instruction in the current frame.
// The returned IrRef can be either a value or an instruction index that maps to a residual
// instruction.
internal IRef resolve(CallFrame *f, SRef ref) {
  if (sref_is_value(ref)) {
    return (IRef){sref_to_u32(ref)};
  }

  return f->inst_map[sref_to_instruction(ref)];
}

internal InstructionIndex
expect_residual_at_instruction_index(CallFrame *f, InstructionIndex inst) {
  IRef res = f->inst_map[inst];

  Assert(iref_is_instruction(res));

  return iref_to_instruction(res);
}

internal void pop_finished_scopes(Specializer *in, CallFrame *f, InstructionIndex end) {
  IIrBuilder *builder = get_builder(in);
  ScopeSpan last;

  while (True) {
    ScopeSpan *span = stack_peek_ptr(&f->scopes);

    if (span->end > end) {
      break;
    }

    if (span->residual) {
      b32 ok = end_residual_block(builder, f, span);
      if (!ok) {
        Todo();
      }
    }

    if (span->condbr) {
      SIrCondbr *condbr = sir_chunk_extra(f->chunk, span->condbr);

      InstructionIndex residual_condbr =
        iref_to_instruction(resolve(f, sref_from_instruction(span->condbr)));

      IIrBuilder *builder = get_builder(in);
      IIrCondbr *data = iir_builder_push_data(builder, residual_condbr, IIrCondbr);
      *data = (IIrCondbr){
        .cond = resolve(f, condbr->cond),
        .then = iref_to_instruction(resolve(f, sref_from_instruction(condbr->then))),
        .otherwise = iref_to_instruction(resolve(f, sref_from_instruction(condbr->otherwise))),
      };
    }

    last = stack_pop(&f->scopes);
  }

  dealloc_scope_values(in, f, last);
}

internal b32
expect_comptime_value_or_nil(Specializer *in, CallFrame *f, SRef ref, ValueIndex *out) {
  IRef x = resolve(f, ref);

  if (iref_is_value(x)) {
    *out = iref_to_value(x);
    return True;
  }

  ScopeSpan *s = stack_peek_ptr(&f->scopes);

  Message_error(
    in->msg_sink,
    (MessageLocation){
      .kind = MessageLocation_ir_instruction,
      .decl_idx = f->decl_idx,
      .data.offset = s->pc,
    },
    string_lit("Value must be comptime known")
  );

  return False;
}

internal b32 expect_some_comptime_value(Specializer *in, CallFrame *f, SRef ref, ValueIndex *out) {
  ValueIndex idx;
  b32 ok = expect_comptime_value_or_nil(in, f, ref, &idx);
  if (!ok) {
    return False;
  }

  ScopeSpan *s = stack_peek_ptr(&f->scopes);

  if (idx == 0) {
    Message_error(
      in->msg_sink,
      (MessageLocation){
        .kind = MessageLocation_ir_instruction,
        .decl_idx = f->decl_idx,
        .data.offset = s->pc,
      },
      string_lit("Value may not be omitted")
    );

    return False;
  }

  *out = idx;

  return True;
}

internal b32 _get_value_expect_type(Specializer *in, CallFrame *f, ValueIndex val, TypeIndex *out) {
  Value *v = values_get(in->values, val);

  ScopeSpan *s = stack_peek_ptr(&f->scopes);

  if (v->type != in->common->type.type) {
    Message_error(
      in->msg_sink,
      (MessageLocation){
        .kind = MessageLocation_ir_instruction,
        .decl_idx = f->decl_idx,
        .data.offset = s->pc,
      },
      string_lit("Expected a type, but got something else")
    );

    return False;
  }

  *out = *Cast(TypeIndex *, v->data);

  return True;
}

internal b32 expect_some_type_value(Specializer *in, CallFrame *f, SRef ref, TypeIndex *out) {
  ValueIndex val;
  b32 ok = expect_some_comptime_value(in, f, ref, &val);
  if (!ok) {
    return False;
  }

  return _get_value_expect_type(in, f, val, out);
}

internal b32 expect_type_value_or_nil(Specializer *in, CallFrame *f, SRef ref, TypeIndex *out) {
  ValueIndex val;
  b32 ok = expect_comptime_value_or_nil(in, f, ref, &val);
  if (!ok) {
    return False;
  }

  if (val == 0) {
    *out = 0;
    return True;
  }

  return _get_value_expect_type(in, f, val, out);
}

internal TypeIndex type_of_val(Specializer *in, ValueIndex v) {
  Assert(v);

  Value *val = values_get(in->values, v);
  return val->type;
}

internal ValueIndex val_from_type(Specializer *in, TypeIndex t) {
  Value *v;
  ValueIndex res = values_alloc(in->values, &v);
  TypeIndex *data = values_alloc_data(in->values, sizeof(TypeIndex), Align_of(TypeIndex));
  *data = t;
  *v = (Value){
    .type = in->common->type.type,
    .data_size = sizeof(TypeIndex),
    .data = data,
  };
  return res;
}

internal TypeIndex ref_typeof(Specializer *in, CallFrame *f, SRef ref) {
  if (sref_is_some_value(ref)) {
    Value *v = values_get(in->values, sref_to_value(ref));
    return v->type;
  }

  InstructionIndex i = sref_to_instruction(ref);

  TypeIndex type = f->inst_types[i];

  Assert(type);

  return type;
}

internal u32 step(Specializer *in, RunState *state) {
  CallFrame *f = top_frame(state);
  ScopeSpan *s = stack_peek_ptr(&f->scopes);

  InstructionIndex pc = s->pc;
  SIrOpcode op = sir_chunk_op(f->chunk, pc);

  switch (op) {
  case SIR_eval_block: {
    Todo();
  } break;

  case SIR_block: {
    if (s->scope_kind == Scope_eval_block) {
      Todo();
    }

    u32 inst_count = sir_chunk_data(f->chunk, pc);

    IIrBuilder *builder = get_builder(in);
    InstructionIndex inst = iir_builder_add(builder, IIR_block);

    store_inst_value(f, pc, iref_from_instruction(inst));

    ScopeSpan *scope = push_scope(f);
    *scope = (ScopeSpan){
      .scope_kind = Scope_block,
      .start = pc,
      .end = pc + inst_count,
      .pc = pc + 1,
      .residual = inst,
    };

    s->pc += inst_count;
  } break;

  case SIR_func: {
    SIrFunc *func = sir_chunk_extra(f->chunk, pc);
    u32 inst_count = func->instruction_count;

    IIrBuilder *builder = push_ir_builder(in);
    InstructionIndex inst_func = iir_builder_add(builder, IIR_func);

    TypeIndex return_type;
    b32 ok = expect_some_type_value(in, f, func->return_type, &return_type);
    if (!ok) {
      return Step_encountered_error;
    }

    iir_builder_set_type(builder, inst_func, return_type);

    for (u32 i = 0; i < func->param_count; i++) {
      Todo(); // set param types
    }

    stack_push(
      &f->scopes,
      ((ScopeSpan){
        .scope_kind = Scope_func,
        .start = pc,
        .end = pc + inst_count,
        .pc = pc + 1,
      })
    );

    s->pc += inst_count;
  } break;

  case SIR_lookup_decl_type: {
    DeclarationIndex decl_idx = sir_chunk_data(f->chunk, pc);
    Declaration *decl = decls_extra_get_ptr(in->declarations, decl_idx);

    if (!state->requested_resolution) {
      if (decl->kind == Declaration_decl) {
        state->requested_resolution = True;
        return Step_resolve_declaration_type;
      }

      Todo();
    }

    Todo();
  } break;

  case SIR_lookup_decl_value: {
    DeclarationIndex decl_idx = sir_chunk_data(f->chunk, pc);
    Declaration *decl = decls_extra_get_ptr(in->declarations, decl_idx);

    if (decl->kind == Declaration_mod) {
      Todo();
    }

    if (!state->requested_resolution && decl->kind == Declaration_decl) {
      state->requested_resolution = True;
      return Step_resolve_declaration_value;
    }

    ValueIndex val;
    if (decl->kind == Declaration_primitive) {
      val = decl->data.primitive;
    } else {
      state->requested_resolution = False;
      val = decl->data.decl.val;
    }

    TypeIndex type;
    {
      Value *v = values_get(in->values, val);
      type = types_add_pointer(in->types, v->type);
    }

    Value *v;
    ValueIndex ptr = values_alloc(in->values, &v);
    ValuePointer *data = values_alloc_data_type(in->values, ValuePointer);

    *data = (ValuePointer){
      .val = val,
      .offset = 0,
    };

    *v = (Value){
      .type = type,
      .data = data,
      .data_size = sizeof(ValuePointer),
    };

    store_inst_value(f, pc, iref_from_value(ptr));

    s->pc += 1;
  } break;

  case SIR_as: {
    SIrAs *as = sir_chunk_extra(f->chunk, pc);
    IRef ref = resolve(f, as->val);

    TypeIndex type_dst;
    b32 ok = expect_some_type_value(in, f, as->type_to, &type_dst);
    if (!ok) {
      return Step_encountered_error;
    }

    f->inst_types[pc] = type_dst;

    // If the value is comptime known, we try to do the coercion right away.
    if (iref_is_some_value(ref)) {
      Value *v = values_get(in->values, iref_to_value(ref));

      ValueIndex val_coerced;
      u32 err = eval_coerce(in->types, in->values, type_dst, v, &val_coerced);
      if (err) {
        if (err == CoerceResult_comptime_int_value_out_of_range) {
          Message_error(
            in->msg_sink,
            (MessageLocation){
              .kind = MessageLocation_ir_instruction,
              .decl_idx = f->decl_idx,
              .data.offset = s->pc,
            },
            string_lit("Value of comptime_int is out of range of destination type")
          );
        }

        if (err == CoerceResult_invalid_coercion_types) {
          Message_error(
            in->msg_sink,
            (MessageLocation){
              .kind = MessageLocation_ir_instruction,
              .decl_idx = f->decl_idx,
              .data.offset = s->pc,
            },
            string_lit("Invalid type coercion")
          );
        }

        return Step_encountered_error;
      }

      store_inst_value(f, pc, iref_from_value(val_coerced));
    } else {
      // as->val is a non-comptime known value, so we can only check if the types are valid for
      // coercion. probably also insert some sort of widening cast that cannot fail

      TypeIndex from = ref_typeof(in, f, as->val);

      if (!is_type_coercible_to(in->types, type_dst, from)) {
        Todo();
      }

      f->inst_types[pc] = type_dst;

      // TODO: check that no widening is necessary for this coercion

      store_inst_value(f, pc, ref);
    }

    s->pc = pc + 1;
  } break;

  case SIR_br: {
    SIrBr *br = sir_chunk_extra(f->chunk, pc);

    if (s->scope_kind == Scope_eval_block) {
      ValueIndex val;
      b32 ok = expect_comptime_value_or_nil(in, f, br->value, &val);
      if (!ok) {
        Todo();
      }

      if (val) {
        f->inst_types[br->block] = type_of_val(in, val);
        val = values_copy(in->values, val);
      } else {
        f->inst_types[br->block] = 0;
      }

      store_inst_value(f, br->block, iref_from_value(val));

      pop_scopes_to(in, f, br->block);

      return Step_leave_scope;
    }

    if (s->scope_kind == Scope_block) {
      TypeIndex type_br;
      IRef ref = resolve(f, br->value);
      if (iref_is_some_value(ref)) {
        ValueIndex val = values_copy(in->values, iref_to_value(ref));
        ref = iref_from_value(val);
        Value *v = values_get(in->values, val);
        type_br = v->type;
      } else {
        type_br = f->inst_types[sref_to_instruction(br->value)];
      }

      f->inst_types[pc] = type_br;

      IIrBuilder *builder = get_builder(in);
      InstructionIndex inst_br = iir_builder_add(builder, IIR_br);
      IIrBr *data_br = iir_builder_push_data(builder, inst_br, IIrBr);
      *data_br = (IIrBr){
        .block = expect_residual_at_instruction_index(f, br->block),
        .value = ref,
      };

      ScopeSpan *block = find_scope(f->scopes.data, f->scopes.len, br->block);
      scope_add_break_or_return(block, pc);

      Assert(s->end == pc + 1); // br may only appear at the end of a block

      f->inst_types[pc] = type_br;
      iir_builder_set_type(builder, inst_br, type_br);

      pop_finished_scopes(in, f, pc + 1);

      return Step_leave_scope;
    }

    Unreachable();
  } break;

  case SIR_type: {
    SIrType *type = sir_chunk_extra(f->chunk, pc);

    TypeIndex t;
    switch (Cast(TypeKind, type->kind)) {
    case Type_function: {
      Assert(type->arg_count >= 1);

      Assert(type->arg_count == 1); // For now don't support params

      TypeIndex return_type;
      b32 ok = expect_type_value_or_nil(in, f, type->args[0], &return_type);
      Assert(ok);
      t = types_add(
        in->types,
        &(Type){
          .kind = Type_function,
          .data.function = {.return_type = return_type, .param_count = 0},
        }
      );

    } break;

    default:
      Panic(); Unreachable();
    }

    ValueIndex v = val_from_type(in, t);
    store_inst_value(f, pc, iref_from_value(v));
    s->pc += 1;
  } break;

  case SIR_unify: {
    SIrUnify *unify = sir_chunk_extra(f->chunk, pc);

    b32 ok = True;

    TypeIndex type_lhs;
    ok = expect_type_value_or_nil(in, f, unify->type_lhs, &type_lhs);
    if (!ok) {
      return Step_encountered_error;
    }

    TypeIndex type_rhs;
    ok = expect_type_value_or_nil(in, f, unify->type_rhs, &type_rhs);
    if (!ok) {
      return Step_encountered_error;
    }

    TypeIndex type_unified;
    u32 err = eval_unify(in->scratch, in->types, type_lhs, type_rhs, &type_unified);
    Assert(!err);

    ValueIndex v = val_from_type(in, type_unified);
    store_inst_value(f, pc, iref_from_value(v));
    s->pc += 1;
  } break;

  case SIR_return_type: {
    SRef ref_func = (SRef){sir_chunk_data(f->chunk, pc)};

    TypeIndex idx;
    b32 ok = expect_some_type_value(in, f, ref_func, &idx);
    Assert(ok);

    Type *t = types_get(in->types, idx);
    ValueIndex v = val_from_type(in, t->data.function.return_type);
    store_inst_value(f, pc, iref_from_value(v));
    s->pc += 1;
  } break;

  case SIR_ret: {
    IIrBuilder *builder = get_builder(in);
    InstructionIndex inst = iir_builder_add(builder, IIR_ret);

    SRef ref_val = (SRef){sir_chunk_data(f->chunk, pc)};
    IRef val = resolve(f, ref_val);

    TypeIndex type_ret;
    if (iref_is_some_value(val)) {
      ValueIndex vidx = values_copy(in->values, iref_to_value(val));
      val = iref_from_value(vidx);
      type_ret = type_of_val(in, vidx);
    } else {
      type_ret = f->inst_types[sref_to_instruction(ref_val)];
    }

    f->inst_types[pc] = type_ret;

    iir_builder_set_data(builder, inst, iref_to_u32(val));
    iir_builder_set_type(builder, inst, type_ret);

    ScopeSpan *func_scope = get_func_scope(f);
    scope_add_break_or_return(func_scope, pc);

    if (pc + 1 == func_scope->end) {
      b32 ok = finalize_function(in, f, func_scope);
      if (!ok) {
        Todo();
      }

      SIrFunc *ir_func = sir_chunk_extra(f->chunk, func_scope->start);

      u32 param_count = ir_func->param_count;

      Type *type = arena_push_type_function(in->scratch, param_count);
      type->kind = Type_function;
      type->data.function.param_count = param_count;

      ok = expect_some_type_value(in, f, ir_func->return_type, &type->data.function.return_type);
      if (!ok) {
        Todo();
      }

      for (u32 i = 0; i < param_count; i++) {
        Todo();
      }

      TypeIndex t = types_add(in->types, type);

      ValueFunc *data = values_alloc_data_type(in->values, ValueFunc);
      Value *v;
      ValueIndex vidx = values_alloc(in->values, &v);
      *v = (Value){
        .type = t,
        .data = data,
        .data_size = sizeof(ValueFunc),
      };

      IIrBuilder *builder = get_builder(in);

      iir_builder_set_data(builder, 0, builder->kinds.len);

      iir_builder_flatten(builder, in->perm, &data->chunk);

      pop_ir_builder(in);

      store_inst_value(f, func_scope->start, iref_from_value(vidx));

      stack_pop(&f->scopes);

      return Step_leave_scope;
    }

    Todo();
  } break;

  case SIR_condbr: {
    SIrCondbr *condbr = sir_chunk_extra(f->chunk, pc);

    IRef cond = resolve(f, condbr->cond);

    if (iref_is_some_value(cond)) {
      Value *v = values_get(in->values, iref_to_value(cond));
      if (*Cast(u8 *, v->data)) {
        s->pc = condbr->then;
      } else {
        s->pc = condbr->otherwise;
      }
    } else {
      IIrBuilder *builder = get_builder(in);

      InstructionIndex inst_condbr = iir_builder_add(builder, IIR_condbr);

      store_inst_value(f, pc, iref_from_instruction(inst_condbr));

      s->condbr = pc;
      s->pc += 1;
    }
  } break;

  case SIR_call: {
    SIrCall *call = sir_chunk_extra(f->chunk, pc);

    IIrBuilder *builder = get_builder(in);

    Assert(call->arg_count == 0);

    InstructionIndex inst_call = iir_builder_add(builder, IIR_call);

    IIrCall *data_call = iir_builder_push_data(builder, inst_call, IIrCall);

    IRef func = resolve(f, call->func);
    if (iref_is_some_value(func)) {
      func = iref_from_value(values_copy(in->values, iref_to_value(func)));
    }

    *data_call = (IIrCall){
      .func_ptr = func,
      .arg_count = 0,
    };

    if (iref_is_some_value(func)) {
      Value *v = values_get(in->values, iref_to_value(func));
      Type *func_type = types_get(in->types, v->type);
      f->inst_types[pc] = func_type->data.function.return_type;
      iir_builder_set_type(builder, inst_call, v->type);
    } else {
      Todo();
    }

    store_inst_value(f, pc, iref_from_instruction(inst_call));

    s->pc += 1;
  } break;

  case SIR_builtin_debug: {
    SRef ref = (SRef){sir_chunk_data(f->chunk, pc)};

    TypeIndex type;
    if (sref_is_some_value(ref)) {
      type = type_of_val(in, sref_to_value(ref));
    } else {
      type = f->inst_types[sref_to_instruction(ref)];
    }

    f->inst_types[pc] = type;

    IRef val = resolve(f, ref);

    if (iref_is_some_value(val)) {
      ValueIndex vidx = values_copy(in->values, iref_to_value(val));
      val = iref_from_value(vidx);
    }

    IIrBuilder *builder = get_builder(in);
    InstructionIndex inst = iir_builder_add(builder, IIR_builtin_debug);
    iir_builder_set_type(builder, inst, type);
    iir_builder_set_data(builder, inst, iref_to_u32(val));

    store_inst_value(f, pc, iref_from_instruction(inst));

    s->pc += 1;
  } break;

  case SIR_load: {
    SRef ref = (SRef){sir_chunk_data(f->chunk, pc)};
    IRef val = resolve(f, ref);

    // TODO make sure it is a pionter

    if (iref_is_some_value(val)) {
      Value *v = values_get(in->values, iref_to_value(val));
      ValuePointer *p = v->data;
      Assert(p->offset == 0);

      ValueIndex x = values_copy(in->values, p->val);

      store_inst_value(f, pc, iref_from_value(x));
    } else {
      IIrBuilder *builder = get_builder(in);
      InstructionIndex inst = iir_builder_add(builder, IIR_load);
      TypeIndex type = iir_builder_get_type(builder, iref_to_instruction(val));
      iir_builder_set_type(builder, inst, type);
      iir_builder_set_data(builder, inst, iref_to_u32(val));

      store_inst_value(f, pc, iref_from_instruction(inst));

      TypeIndex type_idx = ref_typeof(in, f, ref);
      Type *t = types_get(in->types, type_idx);
      Assert(t->kind == Type_pointer);

      f->inst_types[pc] = t->data.pointer.base_type;
    }

    s->pc += 1;
  } break;

  case SIR_store: {
    SIrStore *store = sir_chunk_extra(f->chunk, pc);

    IRef dst = resolve(f, store->dst);

    if (iref_is_some_value(dst)) {
      Todo();
    } else {
      IIrBuilder *builder = get_builder(in);
      InstructionIndex inst = iir_builder_add(builder, IIR_store);
      IIrStore *data = iir_builder_push_data(builder, inst, IIrStore);
      data->ptr = dst;

      IRef val = resolve(f, store->value);

      if (iref_is_some_value(val)) {
        data->value = iref_from_value(values_copy(in->values, iref_to_value(val)));
        iir_builder_set_type(builder, inst, type_of_val(in, iref_to_value(val)));
      } else {
        data->value = val;
        iir_builder_set_type(builder, inst, iir_builder_get_type(builder, iref_to_instruction(val)));
      }
    }

    s->pc += 1;
  } break;

  case SIR_alloc: {
    TypeIndex type;
    b32 ok = expect_some_type_value(in, f, (SRef){sir_chunk_data(f->chunk, pc)}, &type);
    Assert(ok);

    IIrBuilder *builder = get_builder(in);
    InstructionIndex inst = iir_builder_add(builder, IIR_alloc);

    iir_builder_set_type(builder, inst, type);

    store_inst_value(f, pc, iref_from_instruction(inst));

    TypeIndex ptr_type = types_add_pointer(in->types, type);
    f->inst_types[pc] = ptr_type;

    s->pc += 1;
  } break;

  case SIR_comptime_alloc: {
    TypeIndex type;
    b32 ok = expect_some_type_value(in, f, (SRef){sir_chunk_data(f->chunk, pc)}, &type);
    Assert(ok);

    ValueIndex idx_alloc;
    {
      Value *v;
      idx_alloc = values_alloc(in->values, &v);
      TypeSizeInfo size_info = types_size_info_by_index(in->types, type);
      void *data = values_alloc_data(in->values, size_info.size, size_info.align);
      *v = (Value){
        .type = type,
        .data = data,
        .data_size = size_info.size,
      };
    }

    ValueIndex idx_p;
    {
      Value *v;
      idx_p = values_alloc(in->values, &v);
      ValuePointer *data = values_alloc_data_type(in->values, ValuePointer);
      *data = (ValuePointer){
        .val = idx_alloc,
        .offset = 0,
      };
      *v = (Value){
        .type = types_add_pointer(in->types, type),
        .data = data,
        .data_size = sizeof(ValuePointer),
      };
    }

    store_inst_value(f, pc, iref_from_value(idx_p));

    s->pc += 1;
  } break;

  case SIR_base_type: {
    SRef ref = (SRef){sir_chunk_data(f->chunk, pc)};

    TypeIndex type;
    b32 ok = expect_some_type_value(in, f, ref, &type);
    Assert(ok);

    Type *t = types_get(in->types, type);

    TypeIndex base_type;
    switch (t->kind) {
    case Type_pointer:
      base_type = t->data.pointer.base_type;
      break;
    default:
      Todo(); Unreachable();
    }

    ValueIndex v = val_from_type(in, base_type);

    store_inst_value(f, pc, iref_from_value(v));

    s->pc += 1;
  } break;

  case SIR_typeof: {
    IIrBuilder *builder = get_builder(in);

    SRef ref = (SRef){sir_chunk_data(f->chunk, pc)};
    IRef val = resolve(f, ref);

    if (iref_is_some_value(val)) {
      Todo();
    } else {
      InstructionIndex res_idx = iref_to_instruction(val);
      IIrOpcode res_op = iir_builder_get_opcode(builder, res_idx);

      switch (res_op) {
      case IIR_alloc: {
        TypeIndex t = iir_builder_get_type(builder, res_idx);
        TypeIndex ptr_type = types_add_pointer(in->types, t);
        ValueIndex vtype = val_from_type(in, ptr_type);
        store_inst_value(f, pc, iref_from_value(vtype));
      } break;
      default:
        Todo();
      }
    }

    s->pc += 1;
  } break;

  default:
    Todo();
  }

  return Step_ok;
}

u32 run_toplevel_block(Specializer *in, RunState *state) {
  CallFrame *base_frame = state->call_stack.data;

  while (True) {
    CallFrame *f = top_frame(state);

    u32 err = step(in, state);

    if (!err) {
      continue;
    }

    if (err == Step_leave_scope) {
      u32 scope_depth = f->scopes.len;
      if (f == base_frame && scope_depth == 1) {
        return Run_ok;
      }

      continue;
    }

    return err;
  }

  Unreachable();
}
