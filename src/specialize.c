#include "specialize.h"
#include "ir.h"
#include "eval.h"

extern Allocator const cstd_allocator;

internal TypeIndex ref_typeof(Specializer *in, CallFrame *f, IrRef ref);

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

internal IrBuilder *push_ir_builder(Specializer *in) {
  IrBuilder *builder = stack_push_ptr(&in->builders);
  *builder = (IrBuilder){ .scratch = in->scratch };
  return builder;
}

internal void pop_ir_builder(Specializer *in) {
  stack_pop(&in->builders);
}

internal IrBuilder *get_builder(Specializer *in) {
  return stack_peek_ptr(&in->builders);
}

internal always_inline void store_inst_value(CallFrame *f, InstructionIndex idx, ResolvedRef val) {
  f->inst_map[idx] = val;
}

void runstate_init(RunState *state, Arena *arena) {
  state->requested_resolution = False;
  stack_init(&state->call_stack, arena_push_array(CallFrame, arena, MAX_CALL_DEPTH), MAX_CALL_DEPTH);
}

CallFrame *top_frame(RunState *state) {
  return stack_peek_ptr(&state->call_stack);
}

ScopeSpan *get_func_scope(CallFrame *frame) {
  for (u32 i = frame->scopes.len; i-- > 0;) {
    ScopeSpan *s = &frame->scopes.data[i];
    if (s->scope_kind == Scope_func) {
      return s;
    }
  }

  Panic();
}

ScopeSpan *push_scope(CallFrame *frame) {
  return stack_push_ptr(&frame->scopes);
}

internal b32 end_residual_block(IrBuilder *builder, CallFrame *f, ScopeSpan *block) {
  inst_block_end(builder, block->residual);

  if (block->breaks_and_returns.len <= 1) {
    return True;
  }

  TypeIndex type = f->inst_types[block->breaks_and_returns.sources[0]];

  for (u32 i = 1; i < block->breaks_and_returns.len; i++) {
    TypeIndex t = f->inst_types[block->breaks_and_returns.sources[i]];
    if (t != type) {
      Todo();
    }
  }

  return True;
}

internal b32 finalize_function(Specializer *in, CallFrame *f, ScopeSpan *func) {
  IrBuilder *builder = get_builder(in);

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
  IrChunk *chunk = &decl->data.decl.chunk;

  CallFrame *f = stack_push_ptr(&state->call_stack);
  *f = (CallFrame){
    .decl_idx = decl->idx,
    .chunk = chunk,
    .inst_map = arena_push_array(ResolvedRef, arena, chunk->opcode_count),
    .inst_types = arena_push_array(TypeIndex, arena, chunk->opcode_count),
    .snapshot = arena_scope_begin(arena),
  };

  memset(f->inst_map, 0, chunk->opcode_count * sizeof(ResolvedRef));
  memset(f->inst_types, 0, chunk->opcode_count * sizeof(TypeIndex));

  stack_init(&f->scopes, arena_push_array(ScopeSpan, arena, MAX_SCOPE_DEPTH), MAX_SCOPE_DEPTH);

  stack_push(&f->scopes, ((ScopeSpan){
    .scope_kind = Scope_chunk,
    .start = 0,
    .end = f->chunk->opcode_count,
    .pc = 0,
  }));

  return f;
}

void frame_pop(RunState *state, Arena *arena, ValueStore *values) {
  CallFrame f = stack_pop(&state->call_stack);

  ScopeSpan span = f.scopes.data[0];
  for (u32 i = span.start; i < span.end; i++) {
    if (ref_is_some_value_index(f.inst_map[i])) {
      values_dealloc(values, ref_to_value_index(f.inst_map[i]));
    }
  }

  arena_scope_end(arena, f.snapshot);
}

internal void dealloc_scope_values(Specializer *in, CallFrame *f, ScopeSpan span) {
  // Do not dealloc the value stored at the block address
  for (u32 i = span.start + 1; i < span.end; i++) {
    if (ref_is_some_value_index(f->inst_map[i])) {
      values_dealloc(in->values, ref_to_value_index(f->inst_map[i]));
      f->inst_map[i] = (ResolvedRef){ 0 };
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
// The returned IrRef can be either a value or an instruction index that maps to a residual instruction.
internal ResolvedRef resolve(CallFrame *f, IrRef ref) {
  if (ref_is_value_index(ref)) {
    return (ResolvedRef){ ref_to_u32(ref) };
  }

  return f->inst_map[ref_to_instruction_index(ref)];
}

internal InstructionIndex expect_residual_at_instruction_index(CallFrame *f, InstructionIndex inst) {
  ResolvedRef res = f->inst_map[inst];

  Assert(ref_is_instruction_index(res));

  return ref_to_instruction_index(res);
}

internal void pop_finished_scopes(Specializer *in, CallFrame *f, InstructionIndex end) {
  IrBuilder *builder = get_builder(in);
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
      IrCondBr *condbr = chunk_extra(f->chunk, span->condbr);

      InstructionIndex residual_condbr = ref_to_instruction_index(resolve(f, ir_ref_from_instruction_index(span->condbr)));

      IrBuilder *builder = get_builder(in);
      IrCondBr *data = inst_push_data(builder, residual_condbr, IrCondBr);
      *data = (IrCondBr){
        .cond = resolve(f, condbr->cond),
        .then = ref_to_instruction_index(resolve(f, ir_ref_from_instruction_index(condbr->then))),
        .otherwise = ref_to_instruction_index(resolve(f, ir_ref_from_instruction_index(condbr->otherwise))),
      };
    }

    last = stack_pop(&f->scopes);
  }

  dealloc_scope_values(in, f, last);
}

internal b32 expect_comptime_value_or_nil(Specializer *in, CallFrame *f, IrRef ref, ValueIndex *out) {
  ResolvedRef x = resolve(f, ref);

  if (ref_is_value_index(x)) {
    *out = ref_to_value_index(x);
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

internal b32 expect_some_comptime_value(Specializer *in, CallFrame *f, IrRef ref, ValueIndex *out) {
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

  *out = *Cast(TypeIndex*, v->data);

  return True;
}

internal b32 expect_some_type_value(Specializer *in, CallFrame *f, IrRef ref, TypeIndex *out) {
  ValueIndex val;
  b32 ok = expect_some_comptime_value(in, f, ref, &val);
  if (!ok) {
    return False;
  }

  return _get_value_expect_type(in, f, val, out);
}

internal b32 expect_type_value_or_nil(Specializer *in, CallFrame *f, IrRef ref, TypeIndex *out) {
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

internal TypeIndex ref_typeof(Specializer *in, CallFrame *f, IrRef ref) {
  if (ref_is_some_value_index(ref)) {
    Value *v = values_get(in->values, ref_to_value_index(ref));
    return v->type;
  }

  InstructionIndex i = ref_to_instruction_index(ref);

  TypeIndex type = f->inst_types[i];

  Assert(type);

  return type;
}

internal u32 step(Specializer *in, RunState *state) {
  CallFrame *f = top_frame(state);
  ScopeSpan *s = stack_peek_ptr(&f->scopes);

  InstructionIndex pc = s->pc;
  u8 op = chunk_opcode(f->chunk, pc);

  switch (Cast(IrOpcode, op)) {
  case IR_eval_block: {
    Todo();
  } break;
  case IR_block: {
    if (s->scope_kind == Scope_eval_block) {
      Todo();
    }

    u32 inst_count = chunk_data(f->chunk, pc);

    IrBuilder *builder = get_builder(in);
    InstructionIndex inst = inst_alloc(builder);
    inst_set_opcode(builder, inst, IR_block);

    store_inst_value(f, pc, resolved_ref_from_instruction_index(inst));

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
  case IR_func: {
    IrFunc *func = chunk_extra(f->chunk, pc);
    u32 inst_count = func->instruction_count;

    IrBuilder *builder = push_ir_builder(in);
    InstructionIndex inst_func = inst_alloc(builder);
    inst_set_opcode(builder, inst_func, IR_func);
    IrFunc *data_func = inst_push_data(builder, inst_func, IrFunc);
    data_func->param_count = func->param_count;

    TypeIndex return_type;
    b32 ok = expect_some_type_value(in, f, func->return_type, &return_type);
    if (!ok) {
      return Step_encountered_error;
    }

    data_func->return_type = ir_ref_from_value_index(val_from_type(in, return_type));

    for (u32 i = 0; i < func->param_count; i++) {
      Todo(); // set param types
    }

    stack_push(&f->scopes, ((ScopeSpan){
      .scope_kind = Scope_func,
      .start = pc,
      .end   = pc + inst_count,
      .pc    = pc + 1,
    }));

    s->pc += inst_count;
  } break;
  case IR_lookup_typeof: {
    DeclarationIndex decl_idx = chunk_data(f->chunk, pc);
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
  case IR_lookup_value: {
    DeclarationIndex decl_idx = chunk_data(f->chunk, pc);
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

    store_inst_value(f, pc, resolved_ref_from_value_index(ptr));

    s->pc += 1;
  } break;
  case IR_as: {
    IrAs *as = chunk_extra(f->chunk, pc);
    ResolvedRef ref = resolve(f, as->val);

    TypeIndex type_dst;
    b32 ok = expect_some_type_value(in, f, as->type_to, &type_dst);
    if (!ok) {
      return Step_encountered_error;
    }

    f->inst_types[pc] = type_dst;

    // If the value is comptime known, we try to do the coercion right away.
    if (ref_is_some_value_index(ref)) {
      Value *v = values_get(in->values, ref_to_value_index(ref));

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

      store_inst_value(f, pc, resolved_ref_from_value_index(val_coerced));
    } else {
      // as->val is a non-comptime known value, so we can only check if the types are valid for coercion.
      // probably also insert some sort of widening cast that cannot fail

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
  case IR_br: {
    IrBr *br = chunk_extra(f->chunk, pc);

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

      store_inst_value(f, br->block, resolved_ref_from_value_index(val));

      pop_scopes_to(in, f, br->block);

      return Step_leave_scope;
    }

    if (s->scope_kind == Scope_block) {
      TypeIndex type_br;
      ResolvedRef ref = resolve(f, br->value);
      if (ref_is_some_value_index(ref)) {
        ValueIndex val = values_copy(in->values, ref_to_value_index(ref));
        ref = resolved_ref_from_value_index(val);
        Value *v = values_get(in->values, val);
        type_br = v->type;
      } else {
        type_br = f->inst_types[ref_to_instruction_index(br->value)];
      }

      f->inst_types[pc] = type_br;

      IrBuilder *builder = get_builder(in);
      InstructionIndex inst_br = inst_alloc(builder);
      inst_set_opcode(builder, inst_br, IR_br);
      IrBr *data_br = inst_push_data(builder, inst_br, IrBr);
      *data_br = (IrBr){
        .block = expect_residual_at_instruction_index(f, br->block),
        .value = ref,
      };

      ScopeSpan *block = find_scope(f->scopes.data, f->scopes.len, br->block);
      scope_add_break_or_return(block, pc);

      Assert(s->end == pc + 1); // br may only appear at the end of a block

      f->inst_types[pc] = type_br;

      pop_finished_scopes(in, f, pc + 1);

      return Step_leave_scope;
    }

    Unreachable();
  } break;
  case IR_type: {
    IrType *type = chunk_extra(f->chunk, pc);
    TypeIndex t;
    switch (Cast(TypeKind, type->kind)) {
    case Type_function: {
      Assert(type->arg_count >= 1);

      Assert(type->arg_count == 1); // For now don't support params

      TypeIndex return_type;
      b32 ok = expect_type_value_or_nil(in, f, type->args[0], &return_type);
      Assert(ok);
      t = types_add(in->types, &(Type){
        .kind = Type_function,
        .data.function = { .return_type = return_type, .param_count = 0 },
      });
      
    } break;
    default: Panic();
    }
    ValueIndex v = val_from_type(in, t);
    store_inst_value(f, pc, resolved_ref_from_value_index(v));
    s->pc += 1;
  } break;
  case IR_unify: {
    IrUnify *unify = chunk_extra(f->chunk, pc);

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
    store_inst_value(f, pc, resolved_ref_from_value_index(v));
    s->pc += 1;
  } break;
  case IR_return_type: {
    IrRef ref_func = (IrRef){ chunk_data(f->chunk, pc) };

    TypeIndex idx;
    b32 ok = expect_some_type_value(in, f, ref_func, &idx);
    Assert(ok);

    Type *t = types_get(in->types, idx);
    ValueIndex v = val_from_type(in, t->data.function.return_type);
    store_inst_value(f, pc, resolved_ref_from_value_index(v));
    s->pc += 1;
  } break;
  case IR_ret: {
    IrBuilder *builder = get_builder(in);
    InstructionIndex inst = inst_alloc(builder);
    inst_set_opcode(builder, inst, IR_ret);
    IrRef ref_val = (IrRef){ chunk_data(f->chunk, pc) };
    ResolvedRef val = resolve(f, ref_val);

    TypeIndex type_ret;
    if (ref_is_some_value_index(val)) {
      ValueIndex vidx = values_copy(in->values, ref_to_value_index(val));
      val = resolved_ref_from_value_index(vidx);
      type_ret = type_of_val(in, vidx);
    } else {
      type_ret = f->inst_types[ref_to_instruction_index(ref_val)];
    }

    f->inst_types[pc] = type_ret;

    inst_set_data(builder, inst, ref_to_u32(val));

    ScopeSpan *func_scope = get_func_scope(f);
    scope_add_break_or_return(func_scope, pc);

    if (pc + 1 == func_scope->end) {
      b32 ok = finalize_function(in, f, func_scope);
      if (!ok) {
        Todo();
      }

      IrFunc *ir_func = chunk_extra(f->chunk, func_scope->start);

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

      IrBuilder *builder = get_builder(in);

      Cast(IrFunc*, inst_get_extra(builder, 0))->instruction_count = builder->kinds.len;

      irbuilder_flatten(builder, in->perm, &data->chunk);

      pop_ir_builder(in);

      store_inst_value(f, func_scope->start, resolved_ref_from_value_index(vidx));

      stack_pop(&f->scopes);

      return Step_leave_scope;
    }

    Todo();
  } break;
  case IR_condbr: {
    IrCondBr *condbr = chunk_extra(f->chunk, pc);
    ResolvedRef cond = resolve(f, condbr->cond);
    if (ref_is_some_value_index(cond)) {
      Value *v = values_get(in->values, ref_to_value_index(cond));
      if (*Cast(u8*,v->data)) {
        s->pc = condbr->then;
      } else {
        s->pc = condbr->otherwise;
      }
    } else {
      IrBuilder *builder = get_builder(in);

      InstructionIndex inst_condbr = inst_alloc(builder);
      inst_set_opcode(builder, inst_condbr, IR_condbr);

      store_inst_value(f, pc, resolved_ref_from_instruction_index(inst_condbr));

      s->condbr = pc;
      s->pc += 1;
    }
  } break;
  case IR_call: {
    IrCall *call = chunk_extra(f->chunk, pc);
    IrBuilder *builder = get_builder(in);

    Assert(call->arg_count == 0);

    InstructionIndex inst_call = inst_alloc(builder);
    inst_set_opcode(builder, inst_call, IR_call);
    IrCall *data_call = inst_push_data(builder, inst_call, IrCall);

    ResolvedRef func = resolve(f, call->func);
    if (ref_is_some_value_index(func)) {
      func = resolved_ref_from_value_index(values_copy(in->values, ref_to_value_index(func)));
    }

    *data_call = (IrCall){
      .func = func,
      .arg_count = 0,
    };

    if (ref_is_some_value_index(func)) {
      Value *v = values_get(in->values, ref_to_value_index(func));
      Type *func_type = types_get(in->types, v->type);
      f->inst_types[pc] = func_type->data.function.return_type;
    } else {
      Todo();
    }

    store_inst_value(f, pc, resolved_ref_from_instruction_index(inst_call));

    s->pc += 1;
  } break;
  case IR_builtin_debug: {
    IrRef ref = (IrRef){ chunk_data(f->chunk, pc) };

    if (ref_is_some_value_index(ref)) {
      f->inst_types[pc] = type_of_val(in, ref_to_value_index(ref));
    } else {
      f->inst_types[pc] = f->inst_types[ref_to_instruction_index(ref)];
    }

    // if ref == value then the type of this inst is the type of the value.
    // if ref == inst then the type of this inst is the same type of the inst

    ResolvedRef val = resolve(f, ref);

    if (ref_is_some_value_index(val)) {
      ValueIndex vidx = values_copy(in->values, ref_to_value_index(val));
      val = resolved_ref_from_value_index(vidx);
    }

    IrBuilder *builder = get_builder(in);
    InstructionIndex inst = inst_alloc(builder);
    inst_set_opcode(builder, inst, IR_builtin_debug);
    inst_set_data(builder, inst, ref_to_u32(val));

    store_inst_value(f, pc, resolved_ref_from_instruction_index(inst));

    s->pc += 1;
  } break;
  case IR_load: {
    IrRef ref = (IrRef){ chunk_data(f->chunk, pc) };
    ResolvedRef val = resolve(f, ref);

    // TODO make sure it is a pionter

    if (ref_is_some_value_index(val)) {
      Value *v = values_get(in->values, ref_to_value_index(val));
      ValuePointer *p = v->data;
      Assert(p->offset == 0);
      
      ValueIndex x = values_copy(in->values, p->val);

      store_inst_value(f, pc, resolved_ref_from_value_index(x));
    } else {
      IrBuilder *builder = get_builder(in);
      InstructionIndex inst = inst_alloc(builder);
      inst_set_opcode(builder, inst, IR_load);
      inst_set_data(builder, inst, ref_to_u32(val));

      store_inst_value(f, pc, resolved_ref_from_instruction_index(inst));

      TypeIndex type_idx = ref_typeof(in, f, ref);
      Type *t = types_get(in->types, type_idx);
      Assert(t->kind == Type_pointer);

      f->inst_types[pc] = t->data.pointer.base_type;
    }

    s->pc += 1;
  } break;
  case IR_store: {
    IrStore *store = chunk_extra(f->chunk, pc);

    ResolvedRef dst = resolve(f, store->dst);

    if (ref_is_some_value_index(dst)) {
      Todo();
    } else {
      IrBuilder *builder = get_builder(in);
      InstructionIndex inst = inst_alloc(builder);
      inst_set_opcode(builder, inst, IR_store);
      IrStore *store_data = inst_push_data(builder, inst, IrStore);
      store_data->dst = dst;

      ResolvedRef val = resolve(f, store->value);

      if (ref_is_some_value_index(val)) {
        store_data->value = resolved_ref_from_value_index(values_copy(in->values, ref_to_value_index(val)));
      } else {
        Todo();
      }
    }

    s->pc += 1;
  } break;
  case IR_alloc: {
    TypeIndex type;
    b32 ok = expect_some_type_value(in, f, (IrRef){ chunk_data(f->chunk, pc) }, &type);
    Assert(ok);

    IrBuilder *builder = get_builder(in);
    InstructionIndex inst = inst_alloc(builder);
    inst_set_opcode(builder, inst, IR_alloc);

    ValueIndex type_val = val_from_type(in, type);
    inst_set_data(builder, inst, type_val);

    store_inst_value(f, pc, resolved_ref_from_instruction_index(inst));

    TypeIndex ptr_type = types_add_pointer(in->types, type);
    f->inst_types[pc] = ptr_type;

    s->pc += 1;
  } break;
  case IR_comptime_alloc: {
    TypeIndex type;
    b32 ok = expect_some_type_value(in, f, (IrRef){ chunk_data(f->chunk, pc) }, &type);
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

    store_inst_value(f, pc, resolved_ref_from_value_index(idx_p));

    s->pc += 1;
  } break;
  case IR_base_type: {
    IrRef ref = (IrRef){ chunk_data(f->chunk, pc) };
    TypeIndex type;
    b32 ok = expect_some_type_value(in, f, ref, &type); 
    Assert(ok);

    Type *t = types_get(in->types, type);

    TypeIndex base_type;
    switch (t->kind) {
    case Type_pointer: base_type = t->data.pointer.base_type; break;
    default: Todo();
    }
    
    ValueIndex v = val_from_type(in, base_type);

    store_inst_value(f, pc, resolved_ref_from_value_index(v));

    s->pc += 1;
  } break;
  case IR_typeof: {
    IrBuilder *builder = get_builder(in);

    IrRef ref = (IrRef){ chunk_data(f->chunk, pc) };
    ResolvedRef val = resolve(f, ref);

    if (ref_is_some_value_index(val)) {
      Todo();
    } else {
      InstructionIndex res_idx = ref_to_instruction_index(val);
      u8 res_op = inst_get_opcode(builder, res_idx);

      switch (Cast(IrOpcode, res_op)) {
      case IR_alloc: {
        IrRef type_ref = (IrRef){ inst_get_data(builder, res_idx) };
        Assert(ref_is_some_value_index(type_ref));
        Value *v = values_get(in->values, ref_to_value_index(type_ref));
        TypeIndex t = *Cast(TypeIndex*, v->data);
        TypeIndex ptr_type = types_add_pointer(in->types, t);
        ValueIndex vtype = val_from_type(in, ptr_type);
        store_inst_value(f, pc, resolved_ref_from_value_index(vtype));
      } break;
      default: Todo();
      }
    }

    s->pc += 1;
  } break;
  default: Todo();
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
