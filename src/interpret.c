#include "interpret.h"
#include "ir.h"
#include "eval.h"

// The interpreter's per-frame maps grow with the C allocator; cstd_allocator is defined in compiler.c.
extern Allocator const cstd_allocator;

internal TypeIndex ref_typeof(Interpreter *in, ResolvedRef ref);

void scope_add_break_or_return(ScopeSpan *scope, InstructionIndex source, ResolvedRef val) {
  if (scope->breaks_and_returns.len == MAX_BREAKS_AND_RETURNS) {
    Todo();
  }

  u32 i = scope->breaks_and_returns.len++;

  scope->breaks_and_returns.sources[i] = source;
  scope->breaks_and_returns.values[i] = val;
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
    if (s->block_opcode == IR_func) {
      return s;
    }
  }

  Panic();
}

internal b32 finalize_block(Interpreter *in, ScopeSpan *block) {
  TypeIndex type = ref_typeof(in, block->breaks_and_returns.values[0]);

  for (u32 i = 1; i < block->breaks_and_returns.len; i++) {
    Todo();
  }

  return True;
}

internal b32 finalize_function(Interpreter *in, CallFrame *f, ScopeSpan *func) {
  ScopeSpan *s = stack_peek_ptr(&f->scopes);

  while (s != func) {
    b32 ok = finalize_block(in, s);
    if (!ok) {
      return False;
    }

    stack_pop(&f->scopes);
    s = stack_peek_ptr(&f->scopes);
  }

  TypeIndex return_type = ref_typeof(in, func->breaks_and_returns.values[0]);

  for (u32 i = 1; i < func->breaks_and_returns.len; i++) {
    Todo();
  }

  stack_pop(&f->scopes);

  return True;
}

CallFrame *frame_push(RunState *state, Arena *arena, Declaration *decl) {
  IrChunk *chunk = &decl->data.decl.chunk;

  CallFrame *f = stack_push_ptr(&state->call_stack);
  *f = (CallFrame){
    .decl_idx = decl->idx,
    .chunk = chunk,
    .inst_map = arena_push_array(ValueIndex, arena, chunk->opcode_count),
    .snapshot = arena_scope_begin(arena),
  };

  stack_init(&f->scopes, arena_push_array(ScopeSpan, arena, MAX_SCOPE_DEPTH), MAX_SCOPE_DEPTH);

  Assert(chunk->opcodes[0] == IR_eval_block);

  stack_push(&f->scopes, ((ScopeSpan){
    .block_opcode = IR_eval_block,
    .start = 0,
    .end = f->chunk->opcode_count,
    .pc = 1,
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
      string_lit("Value must be comptime known")
    );

    return False;
  }

  *out = idx;

  return True;
}

internal b32 expect_type_value(Interpreter *in, CallFrame *f, ValueIndex val, TypeIndex *out) {
  if (val == 0) {
    Todo();
    return False;
  }

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

internal b32 expect_type_value_or_nil(Interpreter *in, CallFrame *f, ValueIndex val, TypeIndex *out) {
  if (val == 0) {
    *out = 0;
    return True;
  }

  return expect_type_value(in, f, val, out);
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

internal TypeIndex ref_typeof(Interpreter *in, ResolvedRef ref) {
  if (ref_is_some_value_index(ref)) {
    Value *v = values_get(in->values, ref_to_value_index(ref));
    return v->type;
  }

  Todo();
}

internal u32 step(Interpreter *in, RunState *state) {
  CallFrame *f = top_frame(state);
  ScopeSpan *s = stack_peek_ptr(&f->scopes);

  InstructionIndex pc = s->pc;
  u8 op = chunk_opcode(f->chunk, pc);

  switch (Cast(IrOpcode, op)) {
  case IR_eval_block: {
    s->pc += 1;
  } break;
  case IR_block: {
    if (s->block_opcode == IR_eval_block) {
      Todo();
    }

    IrBuilder *builder = get_builder(in);
    InstructionIndex inst = inst_alloc(builder);
    inst_set_opcode(builder, inst, IR_nop);

    store_inst_value(f, s->pc, resolved_ref_from_instruction_index(inst));

    s->pc = pc + 1;
  } break;
  case IR_func: {
    IrFunc *func = chunk_extra(f->chunk, pc);
    u32 inst_count = func->instruction_count;

    IrBuilder *builder = push_ir_builder(in);
    InstructionIndex inst_func = inst_alloc(builder);
    inst_set_opcode(builder, inst_func, IR_func);
    IrFunc *data_func = inst_push_data(builder, inst_func, IrFunc);
    data_func->param_count = func->param_count;

    stack_push(&f->scopes, ((ScopeSpan){
      .block_opcode = IR_func,
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

    ValueIndex idx_dst_type;
    b32 ok = expect_comptime_value(in, f, as->type_to, &idx_dst_type); 
    if (!ok) {
      return Step_encountered_error;
    }

    TypeIndex type_dst;
    ok = expect_type_value(in, f, idx_dst_type, &type_dst);
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

      TypeIndex from = ref_typeof(in, as->val);

      if (!is_type_coercible_to(in->types, type_dst, from)) {
        Todo();
      }

      // TODO: check that no widening is necessary for this coercion

      store_inst_value(f, pc, ref);
    }

    s->pc = pc + 1;
  } break;
  case IR_br: {
    IrBr *br = chunk_extra(f->chunk, pc);

    if (s->block_opcode == IR_eval_block) {
      ValueIndex val;
      b32 ok = expect_comptime_value(in, f, br->value, &val);
      if (!ok) {
        Todo();
      }

      ValueIndex copy = values_copy(in->values, val);
      store_inst_value(f, br->block, resolved_ref_from_value_index(copy));

      stack_pop(&f->scopes);
      break;
    } else {
      Todo();
    }

    //if (ref_is_valid_value_index(val)) {
    //  IrRef copy = ir_ref_from_value_index(values_copy(in->values, ref_to_value_index(val)));

    //  ScopeSpan *scope = Null; // TODO
    //  Todo();

    //  //if (scope->flags & Scope_comptime_eval) {
    //  //  Todo(); // what if the top scope is not the sscope you are breaking to?
    //  //  store_inst_value(f, br->block, val);
    //  //}

    //  scope_add_break_or_return(scope, (ValueSource){ .value = copy, .source = pc });
    //} else {
    //  IrBuilder *builder = get_builder(in);
    //  InstructionIndex inst = inst_alloc(builder);
    //  inst_set_opcode(builder, inst, IR_br);
    //}


    //s->pc = br->block + chunk_data(f->chunk, br->block);
  } break;
  case IR_type: {
    IrType *type = chunk_extra(f->chunk, pc);
    TypeIndex t;
    switch (Cast(TypeKind, type->kind)) {
    case Type_function: {
      Assert(type->arg_count >= 1);

      Assert(type->arg_count == 1); // For now don't support params

      TypeIndex return_type;
      b32 ok = expect_type_value_or_nil(in, f, ref_to_value_index(resolve(f, type->args[0])), &return_type);
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

    ValueIndex lhs;
    ok = expect_comptime_value_or_nil(in, f, unify->type_lhs, &lhs);
    if (!ok) {
      return Step_encountered_error;
    }

    ValueIndex rhs;
    ok = expect_comptime_value_or_nil(in, f, unify->type_rhs, &rhs);
    if (!ok) {
      return Step_encountered_error;
    }

    TypeIndex type_lhs;
    ok = expect_type_value_or_nil(in, f, lhs, &type_lhs);

    TypeIndex type_rhs;
    ok = expect_type_value_or_nil(in, f, rhs, &type_rhs);

    TypeIndex type_unified;
    u32 err = eval_unify(in->scratch, in->types, type_lhs, type_rhs, &type_unified);
    Assert(!err);

    ValueIndex v = val_from_type(in, type_unified);
    store_inst_value(f, pc, resolved_ref_from_value_index(v));
    s->pc += 1;
  } break;
  case IR_return_type: {
    IrRef ref_func = (IrRef){ chunk_data(f->chunk, pc) };

    ValueIndex func;
    b32 ok = expect_comptime_value(in, f, ref_func, &func);
    Assert(ok);

    TypeIndex idx;
    ok = expect_type_value(in, f, func, &idx);
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
    ResolvedRef val = resolve(f, (IrRef){ chunk_data(f->chunk, pc) });

    if (ref_is_some_value_index(val)) {
      val = resolved_ref_from_value_index(values_copy(in->values, ref_to_value_index(val)));
    }

    inst_set_data(builder, inst, ref_to_u32(val));

    ScopeSpan *func_scope = get_func_scope(f);
    scope_add_break_or_return(func_scope, pc, val);

    if (pc + 1 == func_scope->end) {
      b32 ok = finalize_function(in, f, func_scope);
      if (!ok) {
        Todo();
      }

      break;
    }

    Todo();

    ScopeSpan *span = stack_peek_ptr(&f->scopes);
    if (s->pc == span->end) {
      IrFunc *func = inst_get_extra(builder, 0);
      func->instruction_count = inst_offset(builder, 0);

      IrChunk chunk;
      irbuilder_flatten(builder, in->perm, &chunk);

      // TODO pop the builder

      Value *v;
      ValueIndex vidx = values_alloc(in->values, &v);
      ValueFunc *data = values_alloc_data(in->values, sizeof(ValueFunc), Align_of(ValueFunc));
      *data = (ValueFunc){ .chunk = chunk };

      u32 param_count = func->param_count;
      Type *func_type = arena_push_type_function(in->scratch, param_count);
      *func_type = (Type){
        .kind = Type_function,
        .data.function = {
          .return_type = 0,
          .param_count = param_count,
        },
      };

      //Todo(); // TODO set the actual return type and parameter types

      for (u32 i = 0; i < param_count; i++) {
        func_type->data.function.param_types[i] = 0;
      }

      TypeIndex t = types_add(in->types, func_type);

      *v = (Value){
        .type = t,
        .data = data,
        .data_size = sizeof(ValueFunc),
      };

      store_inst_value(f, span->start, resolved_ref_from_value_index(vidx));
    }
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
      IrCondBr *data_condbr = inst_push_data(builder, inst_condbr, IrCondBr);

      Todo();
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

    store_inst_value(f, pc, resolved_ref_from_instruction_index(inst_call));

    s->pc += 1;
  } break;
  default: Todo();
  }

  return Step_ok;
}

u32 run_until(Interpreter *in, RunState *state, u32 end) {
  CallFrame *base_frame = top_frame(state);
  while (True) {
    CallFrame *f = top_frame(state);
    ScopeSpan *s = stack_peek_ptr(&f->scopes);

    if (f == base_frame && s->pc == end) {
      return Run_reached_end;
    }

    u32 err = step(in, state);
    if (err != Step_ok) {
      return err;
    }
  }

  Unreachable();
}
