#include "interpret.h"
#include "ir.h"
#include "eval.h"

// The interpreter's per-frame maps grow with the C allocator; cstd_allocator is defined in compiler.c.
extern Allocator const cstd_allocator;

internal IrBuilder *push_ir_builder(Interpreter *in) {
  IrBuilder *builder = stack_push_ptr(&in->builders);
  *builder = (IrBuilder){ .scratch = in->scratch };
  return builder;
}

internal IrBuilder *get_builder(Interpreter *in) {
  return stack_peek_ptr_unchecked(&in->builders);
}

internal always_inline void store_inst_value(CallFrame *f, InstructionIndex idx, IrRef val) {
  f->inst_map[idx] = val;
}

void runstate_init(RunState *state, Arena *arena) {
  state->requested_resolution = False;
  stack_init(&state->call_stack, arena_push_array(CallFrame, arena, MAX_CALL_DEPTH), MAX_CALL_DEPTH);
}

CallFrame *top_frame(RunState *state) {
  return stack_peek_ptr_unchecked(&state->call_stack);
}

void frame_push(RunState *state, Arena *arena, Declaration *decl) {
  IrChunk *chunk = &decl->data.decl.chunk;
  CallFrame f = {
    .decl_idx = decl->idx,
    .pc = 0,
    .chunk = chunk,
    .inst_map = arena_push_array(ValueIndex, arena, chunk->opcode_count),
    .snapshot = arena_scope_begin(arena),
  };

  stack_init(&f.scopes, arena_push_array(ScopeSpan, arena, MAX_SCOPE_DEPTH), MAX_SCOPE_DEPTH);
  stack_push(&f.scopes, ((ScopeSpan){ .start = 0, .end = chunk->opcode_count }));

  stack_push(&state->call_stack, f);
}

void frame_pop(RunState *state, Arena *arena, ValueStore *values) {
  CallFrame f = stack_pop_unchecked(&state->call_stack);

  ScopeSpan span = f.scopes.data[0];
  for (u32 i = span.start; i < span.end; i++) {
    if (f.inst_map[i]) {
      values_dealloc(values, f.inst_map[i]);
    }
  }

  arena_scope_end(arena, f.snapshot);
}

internal void push_scope(CallFrame *f, u32 inst_count) {
  stack_push(&f->scopes, ((ScopeSpan){ .start = f->pc, .end = f->pc + inst_count }));
}

internal void dealloc_scope_values(Interpreter *in, CallFrame *f, ScopeSpan span) {
  // Do not dealloc the value stored at the block address
  for (u32 i = span.start + 1; i < span.end; i++) {
    if (f->inst_map[i]) {
      values_dealloc(in->values, f->inst_map[i]);
      f->inst_map[i] = 0;
    }
  }
}

internal void pop_scopes_to(Interpreter *in, CallFrame *f, InstructionIndex idx) {
  while (True) {
    ScopeSpan span = stack_pop_unchecked(&f->scopes);

    if (span.start == idx) {
      dealloc_scope_values(in, f, span);
      return;
    }
  }

  Unreachable();
}

// A value-index ref is already a comptime value; an instruction-index ref is the result recorded
// for that instruction in the current frame.
internal IrRef resolve(CallFrame *f, IrRef ref) {
  if (ref_is_value_index(ref)) {
    return ref_to_value_index(ref);
  }

  return f->inst_map[ref_to_instruction_index(ref)];
}

internal b32 expect_comptime_value_or_nil(Interpreter *in, CallFrame *f, IrRef ref, ValueIndex *out) {
  IrRef x = resolve(f, ref);

  if (ref_is_value_index(x)) {
    *out = ref_to_value_index(x);
    return True;
  }

  Message_error(
    in->msg_sink,
    (MessageLocation){
      .kind = MessageLocation_ir_instruction,
      .decl_idx = f->decl_idx,
      .data.offset = f->pc,
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

  if (idx == 0) {
    Message_error(
      in->msg_sink,
      (MessageLocation){
        .kind = MessageLocation_ir_instruction,
        .decl_idx = f->decl_idx,
        .data.offset = f->pc,
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

  if (v->type != in->common->type.type) {
    Message_error(
      in->msg_sink,
      (MessageLocation){
        .kind = MessageLocation_ir_instruction,
        .decl_idx = f->decl_idx,
        .data.offset = f->pc,
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

internal TypeIndex ref_typeof(Interpreter *in, CallFrame *f, IrRef ref) {
  IrRef r = resolve(f, ref);
  if (ref_is_value_index(r) && r) {
    Value *v = values_get(in->values, ref_to_value_index(r));
    return v->type;
  }

  InstructionIndex inst = ref_to_instruction_index(ref);

  IrOpcode op = f->chunk->opcodes[inst];
  switch (Cast(IrOpcode, op)) {
  case IR_call: {
    IrCall *call = chunk_extra(f->chunk, inst);
    TypeIndex func = ref_typeof(in, f, call->func);

    Type *f = types_get(in->types, func);
    return f->data.function.return_type;
  } break;
  default: Todo();
  }

  Todo();
}

internal u32 step(Interpreter *in, RunState *state) {
  CallFrame *f = top_frame(state);

  InstructionIndex pc = f->pc;
  u8 op = chunk_opcode(f->chunk, pc);

  switch (Cast(IrOpcode, op)) {
  case IR_block: {
    IrBuilder *builder = get_builder(in);
    InstructionIndex inst = inst_alloc(builder);
    inst_set_opcode(builder, inst, IR_nop);

    store_inst_value(f, f->pc, ir_ref_from_instruction_index(inst));

    f->pc = pc + 1;
  } break;
  case IR_func: {
    IrFunc *func = chunk_extra(f->chunk, pc);
    u32 inst_count = func->instruction_count;

    IrBuilder *builder = push_ir_builder(in);
    InstructionIndex inst_func = inst_alloc(builder);
    inst_set_opcode(builder, inst_func, IR_func);
    IrFunc *data_func = inst_push_data(builder, inst_func, IrFunc);
    data_func->param_count = func->param_count;

    f->pc += 1;
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
      store_inst_value(f, pc, ir_ref_from_value_index(copy));
      f->pc = pc + 1;
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

      store_inst_value(f, pc, ir_ref_from_value_index(decl->data.decl.val));
      f->pc += 1;
      break;
    }
  } break;
  case IR_as: {
    IrAs *as = chunk_extra(f->chunk, pc);
    IrRef ref = resolve(f, as->val);

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
    if (ref_is_valid_value_index(ref)) {
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
              .data.offset = f->pc,
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
              .data.offset = f->pc,
            },
            string_lit("Invalid type coercion")
          );
        }

        return Step_encountered_error;
      }

      store_inst_value(f, pc, ir_ref_from_value_index(val_coerced));
    } else {
      // as->val is a non-comptime known value, so we can only check if the types are valid for coercion.
      // probably also insert some sort of widening cast that cannot fail

      TypeIndex from = ref_typeof(in, f, as->val);

      if (!is_type_coercible_to(in->types, type_dst, from)) {
        Todo();
      }

      // TODO: check that no widening is necessary for this coercion

      store_inst_value(f, pc, ref);
    }

    f->pc = pc + 1;
  } break;
  case IR_br: {
    IrBr *br = chunk_extra(f->chunk, pc);
    IrRef val = resolve(f, br->value);
    if (ref_is_valid_value_index(val)) {
      val = values_copy(in->values, ref_to_value_index(val));
    } else {
      IrBuilder *builder = get_builder(in);
      InstructionIndex inst = inst_alloc(builder);
      inst_set_opcode(builder, inst, IR_br);
    }
    store_inst_value(f, br->block, val);
    f->pc = br->block + chunk_data(f->chunk, br->block);
  } break;
  case IR_type: {
    IrType *type = chunk_extra(f->chunk, pc);
    TypeIndex t;
    switch (Cast(TypeKind, type->kind)) {
    case Type_function: {
      Assert(type->arg_count >= 1);

      Assert(type->arg_count == 1); // For now don't support params

      TypeIndex return_type;
      b32 ok = expect_type_value_or_nil(in, f, resolve(f, type->args[0]), &return_type);
      Assert(ok);
      t = types_add(in->types, &(Type){
        .kind = Type_function,
        .data.function = { .return_type = return_type, .param_count = 0 },
      });
      
    } break;
    default: Panic();
    }
    ValueIndex v = val_from_type(in, t);
    store_inst_value(f, pc, v);
    f->pc += 1;
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
    store_inst_value(f, pc, v);
    f->pc += 1;
  } break;
  case IR_return_type: {
    IrRef ref_func = chunk_data(f->chunk, pc);

    ValueIndex func;
    b32 ok = expect_comptime_value(in, f, ref_func, &func);
    Assert(ok);

    TypeIndex idx;
    ok = expect_type_value(in, f, func, &idx);
    Assert(ok);

    Type *t = types_get(in->types, idx);
    ValueIndex v = val_from_type(in, t->data.function.return_type);
    store_inst_value(f, pc, v);
    f->pc += 1;
  } break;
  case IR_ret: {
    IrBuilder *builder = get_builder(in);
    InstructionIndex inst = inst_alloc(builder);
    inst_set_opcode(builder, inst, IR_ret);
    IrRef val = resolve(f, chunk_data(f->chunk, pc));
    if (ref_is_value_index(val)) {
      val = values_copy(in->values, val);
    }
    inst_set_data(builder, inst, val);
    f->pc += 1;

    ScopeSpan span = stack_peek_unchecked(&f->scopes);
    if (f->pc == span.end) {
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

      store_inst_value(f, span.start, ir_ref_from_value_index(vidx));
    }
  } break;
  case IR_condbr: {
    IrCondBr *condbr = chunk_extra(f->chunk, pc);
    IrRef cond = resolve(f, condbr->cond);
    if (ref_is_valid_value_index(cond)) {
      Value *v = values_get(in->values, ref_to_value_index(cond));
      if (*Cast(u8*,v->data)) {
        f->pc += 1;
      } else {
        f->pc = condbr->otherwise;
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

    IrRef func = resolve(f, call->func);

    *data_call = (IrCall){
      .func = func,
      .arg_count = 0,
    };

    store_inst_value(f, pc, ir_ref_from_instruction_index(inst_call));

    f->pc += 1;
  } break;
  default: Todo();
  }

  return Step_ok;
}

u32 run_until(Interpreter *in, RunState *state, u32 end) {
  CallFrame *base_frame = top_frame(state);
  while (True) {
    CallFrame *f = top_frame(state);

    if (f == base_frame && f->pc == end) {
      return Run_reached_end;
    }

    u32 err = step(in, state);
    if (err != Step_ok) {
      return err;
    }
  }

  Unreachable();
}
