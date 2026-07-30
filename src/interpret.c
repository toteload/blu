#include "interpret.h"
#include "ir.h"
#include "source_file.h"
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

void frame_push(CallStack *call_stack, Arena *arena, IrChunk *chunk) {
  CallFrame f = {
    .ok = True,
    .pc = 0,
    .chunk = chunk,
    .inst_map = arena_push_array(ValueIndex, arena, chunk->opcode_count),
    .snapshot = arena_scope_begin(arena),
  };

  stack_init(&f.scopes, arena_push_array(ScopeSpan, arena, MAX_SCOPE_DEPTH), MAX_SCOPE_DEPTH);
  stack_push(&f.scopes, ((ScopeSpan){ .start = 0, .end = chunk->opcode_count }));

  stack_push(call_stack, f);
}

void frame_pop(CallStack *call_stack, Arena *arena, ValueStore *values) {
  CallFrame f = stack_pop_unchecked(call_stack);

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

// This function returns a nil if the ref doesn't refer to a comptime value.
internal ValueIndex try_ref_as_value(CallFrame *f, IrRef ref) {
  IrRef x = resolve(f, ref);
  if (ref_is_value_index(x)) {
    return ref_to_value_index(x);
  }

  return 0;
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

internal u32 step(Interpreter *in, CallStack *stack, b32 reentry) {
  CallFrame *f = stack_peek_ptr_unchecked(stack);

  InstructionIndex pc = f->pc;
  u8 op = chunk_opcode(f->chunk, pc);

  switch (Cast(enum IrOpcode, op)) {
  case IR_block: {
    u32 inst_count = chunk_data(f->chunk, pc);
    push_scope(f, inst_count);
    f->pc = pc + 1;
  } break;
  case IR_func: {
    IrFunc *func = chunk_extra(f->chunk, pc);
    u32 inst_count = func->instruction_count;
    push_scope(f, inst_count);

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

    if (!reentry) {
      if (decl->kind == Declaration_decl) {
        return Run_resolve_declaration_type;
      }

      Todo();
    }

    Todo();
  } break;
  case IR_lookup_value: {
    DeclarationIndex decl_idx = chunk_data(f->chunk, pc);
    Declaration *decl = decls_extra_get_ptr(in->declarations, decl_idx);

    if (!reentry) {
      if (decl->kind == Declaration_decl) {
        return Run_resolve_declaration_value;
      }

      if (decl->kind == Declaration_primitive) {
        ValueIndex copy = values_copy(in->values, decl->data.primitive);
        store_inst_value(f, pc, ir_ref_from_value_index(copy));
        f->pc = pc + 1;
        break;
      }

      Todo();
    }

    Todo();
  } break;
  case IR_as: {
    IrAs *as = chunk_extra(f->chunk, pc);
    IrRef ref = resolve(f, as->val);

    IrRef res;

    // If the value is comptime known, we try to do the coercion right away.
    if (ref_is_value_index(ref)) {
      Value *v = values_get(in->values, ref_to_value_index(ref));

      ValueIndex idx_dst_type = try_ref_as_value(f, as->type_to); 
      if (!idx_dst_type) {
        Panic();
      }

      TypeIndex type_dst = type_from_val(in, idx_dst_type);

      ValueIndex val_coerced;
      u32 err = eval_coerce(in->types, in->values, type_dst, v, &val_coerced);
      f->ok = !err;

      Assert(!err);

      res = ir_ref_from_value_index(val_coerced);
    } else {
      // as->val is a non-comptime known value, so we can only check if the types are valid for coercion.
      // probably also insert some sort of widening cast that cannot fail
      Todo();
    }

    store_inst_value(f, pc, res);

    f->pc = pc + 1;
  } break;
  case IR_br: {
    IrBr *br = chunk_extra(f->chunk, pc);
    IrRef val = resolve(f, br->value);
    if (ref_is_value_index(val)) {
      val = values_copy(in->values, ref_to_value_index(val));
    }
    store_inst_value(f, br->block, val);
    pop_scopes_to(in, f, br->block);
    f->pc = br->block + chunk_data(f->chunk, br->block);
  } break;
  case IR_type: {
    IrType *type = chunk_extra(f->chunk, pc);
    TypeIndex t;
    switch (Cast(enum TypeKind, type->kind)) {
    case Type_function: {
      Assert(type->arg_count >= 1);

      Assert(type->arg_count == 1); // For now don't support params
      TypeIndex return_type = type_from_val(in, resolve(f, type->args[0]));
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

    ValueIndex lhs = try_ref_as_value(f, unify->type_lhs);
    Assert(lhs);

    ValueIndex rhs = try_ref_as_value(f, unify->type_rhs);
    Assert(rhs);

    TypeIndex type_lhs = type_from_val(in, lhs);
    TypeIndex type_rhs = type_from_val(in, rhs);

    TypeIndex type_unified;
    u32 err = eval_unify(in->scratch, in->types, type_lhs, type_rhs, &type_unified);
    Assert(!err);

    ValueIndex v = val_from_type(in, type_unified);
    store_inst_value(f, pc, v);
    f->pc += 1;
  } break;
  case IR_return_type: {
    IrRef ref_func = chunk_data(f->chunk, pc);

    ValueIndex func = try_ref_as_value(f, ref_func);
    Assert(func);

    // TODO: make sure func is a type

    TypeIndex idx = type_from_val(in, func);
    Assert(idx);

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
      pop_scopes_to(in, f, span.start);

      // TODO pop the builder

      Value *v;
      ValueIndex vidx = values_alloc(in->values, &v);
      ValueFunc *data = values_alloc_data(in->values, sizeof(ValueFunc), Align_of(ValueFunc));
      *data = (ValueFunc){ .chunk = chunk };

      u32 param_count = 0; // TODO set actual param count
      Type *func_type = arena_push_type_function(in->scratch, param_count);
      *func_type = (Type){
        .kind = Type_function,
        .data.function = {
          .return_type = 0,
          .param_count = param_count,
        },
      };

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
  default: Panic();
  }

  return Run_ok;
}

u32 run_until(Interpreter *in, CallStack *stack, u32 end, b32 reentry) {
  u32 base_idx = stack->len;

  while (True) {
    CallFrame *f = stack_peek_ptr_unchecked(stack);
    if (stack->len == base_idx && f->pc == end) {
      break;
    }

    step(in, stack, reentry);
  }

  return Run_ok;
}
