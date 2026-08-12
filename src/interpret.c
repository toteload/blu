#include "interpret.h"
#include "ir.h"
#include "eval.h"

// The interpreter's per-frame maps grow with the C allocator; cstd_allocator is defined in compiler.c.
extern Allocator const cstd_allocator;

internal TypeIndex ref_typeof(Interpreter *in, CallFrame *f, IrRef ref);

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

internal IrBuilder *push_ir_builder(Interpreter *in) {
  IrBuilder *builder = stack_push_ptr(&in->builders);
  *builder = (IrBuilder){ .scratch = in->scratch };
  return builder;
}

internal IrBuilder *get_builder(Interpreter *in) {
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

internal b32 finalize_block(CallFrame *f, ScopeSpan *block) {
  if (block->breaks_and_returns.len <= 1) {
    return True;
  }

  TypeIndex type = f->inst_types[block->breaks_and_returns.sources[0]];

  for (u32 i = 1; i < block->breaks_and_returns.len; i++) {
    Todo();
  }

  return True;
}

internal b32 finalize_function(Interpreter *in, CallFrame *f, ScopeSpan *func) {
  ScopeSpan *s = stack_peek_ptr(&f->scopes);

  while (s != func) {
    b32 ok = finalize_block(f, s);
    if (!ok) {
      return False;
    }

    stack_pop(&f->scopes);
    s = stack_peek_ptr(&f->scopes);
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

internal void dealloc_scope_values(Interpreter *in, CallFrame *f, ScopeSpan span) {
  // Do not dealloc the value stored at the block address
  for (u32 i = span.start + 1; i < span.end; i++) {
    if (ref_is_some_value_index(f->inst_map[i])) {
      values_dealloc(in->values, ref_to_value_index(f->inst_map[i]));
      f->inst_map[i] = (ResolvedRef){ 0 };
    }
  }
}

internal void pop_scopes_to(Interpreter *in, CallFrame *f, InstructionIndex idx) {
  while (True) {
    ScopeSpan span = stack_pop(&f->scopes);

    if (span.start == idx) {
      dealloc_scope_values(in, f, span);
      return;
    }
  }

  Unreachable();
}

internal void pop_finished_scopes(Interpreter *in, CallFrame *f, InstructionIndex end) {
  ScopeSpan last;

  while (True) {
    ScopeSpan *span = stack_peek_ptr(&f->scopes);

    if (span->end > end) {
      break;
    }

    last = stack_pop(&f->scopes);
  }

  dealloc_scope_values(in, f, last);
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

internal b32 expect_comptime_value_or_nil(Interpreter *in, CallFrame *f, IrRef ref, ValueIndex *out) {
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

internal b32 expect_comptime_value(Interpreter *in, CallFrame *f, IrRef ref, ValueIndex *out) {
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

internal b32 _get_value_expect_type(Interpreter *in, CallFrame *f, ValueIndex val, TypeIndex *out) {
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

internal b32 expect_type_value(Interpreter *in, CallFrame *f, IrRef ref, TypeIndex *out) {
  ValueIndex val;
  b32 ok = expect_comptime_value(in, f, ref, &val);
  if (!ok) {
    return False;
  }

  return _get_value_expect_type(in, f, val, out);
}

internal b32 expect_type_value_or_nil(Interpreter *in, CallFrame *f, IrRef ref, TypeIndex *out) {
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

// ASSUME: `v` refers to a TypeIndex
internal TypeIndex type_from_val(Interpreter *in, ValueIndex v) {
  if (v == 0) {
    return 0;
  }

  Value *val = values_get(in->values, v);
  TypeIndex *t = val->data;
  return *t;
}

internal ValueIndex val_from_type(Interpreter *in, TypeIndex t) {
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

internal TypeIndex ref_typeof(Interpreter *in, CallFrame *f, IrRef ref) {
  if (ref_is_some_value_index(ref)) {
    Value *v = values_get(in->values, ref_to_value_index(ref));
    return v->type;
  }

  InstructionIndex i = ref_to_instruction_index(ref);

  TypeIndex type = f->inst_types[i];

  Assert(type);

  return type;

  //IrBuilder *builder = get_builder(in);

  //u8 op = inst_opcode(builder, i);

  //switch (Cast(IrOpcode, op)) {
  //case IR_call: {
  //  IrCall *call = inst_get_extra(builder, i);
  //  TypeIndex func_type_idx = ref_typeof(in, call->func);
  //  Type *func_type = types_get(in->types, func_type_idx);
  //  return func_type->data.function.return_type;
  //} break;
  //case IR_block: {
  //  Todo();
  //} break;
  //default: Todo();
  //}

  //Todo();
}

internal u32 step(Interpreter *in, RunState *state) {
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

    store_inst_value(f, s->pc, resolved_ref_from_instruction_index(inst));

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
    b32 ok = expect_type_value(in, f, func->return_type, &return_type);
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

    if (decl->kind == Declaration_primitive) {
      ValueIndex copy = values_copy(in->values, decl->data.primitive);
      store_inst_value(f, pc, resolved_ref_from_value_index(copy));
      s->pc = pc + 1;
      break;
    }

    if (!state->requested_resolution) {
      if (decl->kind == Declaration_decl) {
        state->requested_resolution = True;
        return Step_resolve_declaration_value;
      }

      Todo();
    } else {
      state->requested_resolution = False;
      Assert(decl->kind == Declaration_decl);

      store_inst_value(f, pc, resolved_ref_from_value_index(decl->data.decl.val));
      s->pc += 1;
      break;
    }
  } break;
  case IR_as: {
    IrAs *as = chunk_extra(f->chunk, pc);
    ResolvedRef ref = resolve(f, as->val);

    TypeIndex type_dst;
    b32 ok = expect_type_value(in, f, as->type_to, &type_dst);
    if (!ok) {
      return Step_encountered_error;
    }

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
      b32 ok = expect_comptime_value(in, f, br->value, &val);
      if (!ok) {
        Todo();
      }

      ValueIndex copy = values_copy(in->values, val);
      store_inst_value(f, br->block, resolved_ref_from_value_index(copy));

      f->inst_types[br->block] = type_from_val(in, val);

      pop_scopes_to(in, f, br->block);

      return Step_leave_scope;
    }

    if (s->scope_kind == Scope_block) {
      TypeIndex type_br;
      ResolvedRef ref = resolve(f, br->value);
      if (ref_is_some_value_index(ref)) {
        ValueIndex val = values_copy(in->values, ref_to_value_index(ref));
        ref = resolved_ref_from_value_index(val);
        type_br = type_from_val(in, val);
      } else {
        type_br = f->inst_types[ref_to_instruction_index(br->value)];
      }

      f->inst_types[pc] = type_br;

      IrBuilder *builder = get_builder(in);
      InstructionIndex inst_br = inst_alloc(builder);
      inst_set_opcode(builder, inst_br, IR_br);
      IrBr *data_br = inst_push_data(builder, inst_br, IrBr);
      *data_br = (IrBr){
        .block = s->residual,
        .value = ref,
      };

      ScopeSpan *block = find_scope(f->scopes.data, f->scopes.len, br->block);
      scope_add_break_or_return(block, pc);

      Assert(s->end == pc + 1); // br may only appear at the end of a block

      b32 ok = finalize_block(f, s);
      if (!ok) {
        Todo();
      }

      f->inst_types[br->block] = type_br;

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
    b32 ok = expect_type_value(in, f, ref_func, &idx);
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
      type_ret = type_from_val(in, vidx);
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

      ok = expect_type_value(in, f, ir_func->return_type, &type->data.function.return_type);
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

      if (s->flags & Scope_condbr_has_evaluated_branches) {
        InstructionIndex inst_condbr = ref_to_instruction_index(resolve(f, ir_ref_from_instruction_index(pc)));
        IrCondBr *data_condbr = inst_push_data(builder, inst_condbr, IrCondBr);
        *data_condbr = (IrCondBr){
          .cond = cond,
          .then = ref_to_instruction_index(resolve(f, ir_ref_from_instruction_index(condbr->then))),
          .otherwise = ref_to_instruction_index(resolve(f, ir_ref_from_instruction_index(condbr->otherwise))),
        };
        s->pc += 1;
        break;
      }

      s->flags |= Scope_condbr_has_evaluated_branches;

      InstructionIndex inst_condbr = inst_alloc(builder);
      inst_set_opcode(builder, inst_condbr, IR_condbr);

      store_inst_value(f, pc, resolved_ref_from_instruction_index(inst_condbr));

      {
        u32 inst_count = chunk_data(f->chunk, condbr->otherwise);
        InstructionIndex start = condbr->otherwise;
        ScopeSpan *scope = push_scope(f);
        *scope = (ScopeSpan){
          .scope_kind = Scope_block,
          .start = start,
          .end = start + inst_count,
          .pc = start + 1,
        };
      }

      {
        u32 inst_count = chunk_data(f->chunk, condbr->then);
        InstructionIndex start = condbr->then;
        ScopeSpan *scope = push_scope(f);
        *scope = (ScopeSpan){
          .scope_kind = Scope_block,
          .start = start,
          .end = start + inst_count,
          .pc = start + 1,
        };
      }
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
  default: Todo();
  }

  return Step_ok;
}

u32 run_block(Interpreter *in, RunState *state) {
  CallFrame *base_frame = top_frame(state);
  u32 target_scope_depth = base_frame->scopes.len - 1;

  while (True) {
    CallFrame *f = top_frame(state);

    u32 err = step(in, state);

    if (!err) {
      continue;
    }

    if (err == Step_leave_scope) {
      u32 scope_depth = f->scopes.len;
      if (f == base_frame && scope_depth == target_scope_depth) {
        return Run_reached_end;
      }

      continue;
    }

    return err;
  }

  Unreachable();
}
