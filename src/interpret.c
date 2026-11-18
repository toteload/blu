#include "interpret.h"
#include "eval.h"
#include "print.h"

#define MAX_SCOPE_DEPTH 64

internal TypeSizeInfo layout_of(Interpreter *in, TypeKey type) {
  if (key_is_nil(type)) {
    return (TypeSizeInfo){ .size = 0, .stride = 0, .align = 1 };
  }

  return types_value_layout_by_key(&in->compiler->types, type);
}

internal ValueKey follow_stubs(Compiler *compiler, ValueKey key) {
  Value *v = values_get(&compiler->values, key);

  while (key_eq(v->type, TYPE_STUB)) {
    Stub *stub = compiler_get_stub(compiler, Cast(ValueStubRef *, v->data)->stub);
    Assert(stub->resolve_status == RESOLVE_STATUS_FULLY_RESOLVED);
    key = stub->val;
    v = values_get(&compiler->values, key);
  }

  return key;
}

internal ValueOffset memory_alloc(Interpreter *in, TypeSizeInfo info) {
  void *p = arena_push(in->memory, info.size, info.align);
  return Cast(ValueOffset, ptr_diff(p, in->memory->base));
}

internal void *memory_ptr(Interpreter *in, ValueOffset offset) {
  return ptr_offset(in->memory->base, offset);
}

internal void *value_data(Interpreter *in, ValueKey key) {
  return values_get(&in->compiler->values, follow_stubs(in->compiler, key))->data;
}

internal void *deref(Interpreter *in, ValuePointer p) {
  void *base = vref_is_value(p.base)
    ? value_data(in, vref_to_value(p.base))
    : memory_ptr(in, vref_to_offset(p.base));
  return ptr_offset(base, p.offset);
}

internal void *slot_ptr(Interpreter *in, CallFrame2 *f, InstructionOffset inst) {
  return memory_ptr(in, f->value_offsets[inst]);
}

internal void *resolve(Interpreter *in, CallFrame2 *f, IRef ref) {
  if (iref_is_value(ref)) {
    return value_data(in, iref_to_value(ref));
  }

  return slot_ptr(in, f, iref_to_instruction(ref));
}

internal TypeKey ref_type(Interpreter *in, CallFrame2 *f, IRef ref) {
  if (iref_is_value(ref)) {
    ValueKey key = follow_stubs(in->compiler, iref_to_value(ref));
    return values_get(&in->compiler->values, key)->type;
  }

  return iir_chunk_type(f->chunk, iref_to_instruction(ref));
}

internal void copy_value(void *dst, void const *src, u32 size) {
  if (size > 0 && src) {
    memcpy(dst, src, size);
  }
}

internal CallFrame2 *frame_push(Interpreter *in, IIrChunk *chunk, void *ret) {
  ArenaSnapshot snapshot = arena_scope_begin(in->memory);

  CallFrame2 *f = stack_push_ptr(&in->call_stack);

  u32 count = chunk->opcode_count;

  f->ret = ret;
  f->chunk = chunk;
  f->value_offsets = arena_push_array(u32, in->memory, count);

  stack_init(&f->scope_stack, arena_push_array(ScopeSpan2, in->memory, MAX_SCOPE_DEPTH), MAX_SCOPE_DEPTH);
  stack_push(&f->scope_stack, ((ScopeSpan2){ .start = 0, .end = count, .snapshot = snapshot }));

  for (u32 i = 1; i < count; i++) {
    if (iir_chunk_op(chunk, i) != IIR_param) {
      f->pc = i;
      break;
    }
  }

  return f;
}

internal void frame_set_param(Interpreter *in, CallFrame2 *f, u32 param_index, void const *src) {
  InstructionOffset inst = 1 + param_index;
  Assert(iir_chunk_op(f->chunk, inst) == IIR_param);

  TypeSizeInfo size_info = layout_of(in, iir_chunk_type(f->chunk, inst));
  f->value_offsets[inst] = memory_alloc(in, size_info);
  copy_value(slot_ptr(in, f, inst), src, size_info.size);
}

internal void frame_pop(Interpreter *in) {
  CallFrame2 *f = stack_peek_ptr(&in->call_stack);

  ScopeSpan2 *span = f->scope_stack.data;
  arena_scope_end(in->memory, span->snapshot);

  stack_pop_unchecked(&in->call_stack);
}

internal void scope_push(Interpreter *in, CallFrame2 *f, InstructionOffset block) {
  u32 inst_count = iir_chunk_data(f->chunk, block);
  stack_push(&f->scope_stack, ((ScopeSpan2){
    .start = block,
    .end = block + inst_count,
    .snapshot = arena_scope_begin(in->memory),
  }));
}

internal void scopes_pop_to(Interpreter *in, CallFrame2 *f, InstructionOffset block) {
  while (True) {
    ScopeSpan2 span = stack_pop(&f->scope_stack);

    if (span.start == block) {
      arena_scope_end(in->memory, span.snapshot);
      return;
    }
  }

  Unreachable();
}

typedef enum {
  Step_ok,
  Step_return,
  Step_integer_overflow,
  Step_zero_division,
  Step_index_out_of_bounds,
  Step_illegal_opcode,
} StepResult;

internal MessageLocation inst_location(CallFrame2 *f, InstructionOffset pc) {
  AstAndSourceKey src = f->chunk->sources[pc];
  if (key_is_nil(src.ast_idx)) {
    return (MessageLocation){ .kind = MessageLocation_unspecified, };
  }

  return (MessageLocation){
    .kind = MessageLocation_ast_index,
    .source_idx = src.source_idx,
    .data.ast_index = src.ast_idx,
  };
}

internal u32 step(Interpreter *in) {
  CallFrame2 *f = stack_peek_ptr(&in->call_stack);
  InstructionOffset pc = f->pc;
  IIrOpcode op = iir_chunk_op(f->chunk, pc);
  TypeKey type = iir_chunk_type(f->chunk, pc);
  TypeSizeInfo size_info = layout_of(in, type);

  f->value_offsets[pc] = memory_alloc(in, size_info);
  void *local = slot_ptr(in, f, pc);

  switch (op) {
  case IIR_func: {
    Message_error(
      in->msg_sink,
      inst_location(f, pc),
      string_lit("Stepped into IIR_func instruction which is only allowed to be called")
    );
    return Step_illegal_opcode;
  } break;

  case IIR_param: {
    Message_error(
      in->msg_sink,
      inst_location(f, pc),
      string_lit("Stepped into IIR_param instruction which should be set by caller")
    );
    return Step_illegal_opcode;
  } break;

  case IIR_loop:
  case IIR_block: {
    scope_push(in, f, pc);
    f->pc += 1;
  } break;

  case IIR_repeat: {
    InstructionOffset loop = iir_chunk_data(f->chunk, pc);
    scopes_pop_to(in, f, loop);
    scope_push(in, f, loop);
    f->pc = loop + 1;
  } break;

  case IIR_br: {
    IIrBr *br = iir_chunk_extra(f->chunk, pc);
    u32 size = layout_of(in, iir_chunk_type(f->chunk, br->block)).size;
    copy_value(slot_ptr(in, f, br->block), resolve(in, f, br->value), size);
    scopes_pop_to(in, f, br->block);
    u32 inst_count = iir_chunk_data(f->chunk, br->block);
    f->pc = br->block + inst_count;
  } break;

  case IIR_br_void: {
    InstructionOffset block = iir_chunk_data(f->chunk, pc);
    scopes_pop_to(in, f, block);
    u32 inst_count = iir_chunk_data(f->chunk, block);
    f->pc = block + inst_count;
  } break;

  case IIR_condbr: {
    IIrCondbr *condbr = iir_chunk_extra(f->chunk, pc);
    u8* v = resolve(in, f, condbr->cond);
    if (*v) {
      f->pc += 1;
    } else {
      f->pc = condbr->otherwise;
    }
  } break;

  case IIR_call: {
    IIrCall *call = iir_chunk_extra(f->chunk, pc);
    ResidualFunctionKey key = *Cast(ResidualFunctionKey *, resolve(in, f, call->func_ptr));
    IIrChunk *chunk = &compiler_get_function(in->compiler, key)->chunk;

    f->pc += 1;

    CallFrame2 *g = frame_push(in, chunk, local);

    for (u32 i = 0; i < call->arg_count; i++) {
      frame_set_param(in, g, i, resolve(in, f, call->args[i]));
    }
  } break;

  case IIR_ret: {
    void *src = resolve(in, f, iir_chunk_ref(f->chunk, pc));
    copy_value(f->ret, src, size_info.size);
    frame_pop(in);
    return Step_return;
  } break;

  case IIR_ret_void: {
    frame_pop(in);
    return Step_return;
  } break;

  case IIR_index: {
    IIrBinary *bin = iir_chunk_extra(f->chunk, pc);

    Type *t = types_get(&in->compiler->types, ref_type(in, f, bin->lhs));

    void *base;
    u64 len;
    if (t->kind == Type_array) {
      // An array's elements are stored inline, so the operand is already the element data.
      base = resolve(in, f, bin->lhs);
      len = t->data.array.size;
    } else {
      ValueSlice *slice = resolve(in, f, bin->lhs);
      base = deref(in, slice->ptr);
      len = slice->len;
    }

    u64 at = read_int_zero_extend(64, resolve(in, f, bin->rhs));

    if (at >= len) {
      Message_error(
        in->msg_sink,
        inst_location(f, pc),
        string_lit("Index is out of bounds. index = %u64, bounds = 0 .. %u64"),
        at, len);
      return Step_index_out_of_bounds;
    }

    u32 stride = types_value_layout_by_key(&in->compiler->types, type).stride;
    copy_value(local, ptr_offset(base, at * stride), size_info.size);

    f->pc += 1;
  } break;

  case IIR_builtin_len: {
    ValueSlice *slice = resolve(in, f, iir_chunk_ref(f->chunk, pc));
    *Cast(usize *, local) = slice->len;
    f->pc += 1;
  } break;

  case IIR_builtin_debug: {
    void *p = resolve(in, f, iir_chunk_ref(f->chunk, pc));
    copy_value(local, p, size_info.size);
    print_value_raw(stdout, in->compiler, 0, type, p);
    fputs("\n", stdout);
    f->pc += 1;
  } break;

  case IIR_alloc: {
    Type *t = types_get(&in->compiler->types, type);
    Assert(t->kind == Type_pointer);
    ValueOffset offset = memory_alloc(in, layout_of(in, t->data.pointer.base_type));
    *Cast(ValuePointer *, local) = (ValuePointer){ .base = vref_from_offset(offset), .offset = 0 };
    f->pc += 1;
  } break;

  case IIR_global: {
    *Cast(ValuePointer *, local) = (ValuePointer){ .base = (ValueRef){iir_chunk_data(f->chunk, pc)}, .offset = 0 };
    f->pc += 1;
  } break;

  case IIR_store: {
    IIrStore *store = iir_chunk_extra(f->chunk, pc);
    ValuePointer *p = resolve(in, f, store->ptr);
    Assert(vref_is_offset(p->base));
    copy_value(deref(in, *p), resolve(in, f, store->value), size_info.size);
    f->pc += 1;
  } break;

  case IIR_load: {
    ValuePointer *p = resolve(in, f, iir_chunk_ref(f->chunk, pc));
    copy_value(local, deref(in, *p), size_info.size);
    f->pc += 1;
  } break;

  case IIR_int_extend:
  case IIR_int_cast_check: {
    IRef ref = iir_chunk_ref(f->chunk, pc);

    // The operand of an int_cast is always runtime otherwise the cast would have already been performed in the specializer.
    Assert(iref_is_instruction(ref));

    void *val = resolve(in, f, ref);

    Type *t = types_get(&in->compiler->types, ref_type(in, f, ref));
    Type *v = types_get(&in->compiler->types, type);

    u32 err = eval_cast_int(t->data.integer, val, v->data.integer, local);
    if (err) {
      Message_error(
        in->msg_sink,
        inst_location(f, pc),
        string_lit("Value does not fit in the destination type of the cast")
      );
      return Step_integer_overflow;
    }

    f->pc += 1;
  } break;

  case IIR_int_add: {
    IIrBinary *bin = iir_chunk_extra(f->chunk, pc);
    void *lhs = resolve(in, f, bin->lhs);
    void *rhs = resolve(in, f, bin->rhs);

    Type *t = types_get(&in->compiler->types, type);
    b32 ok = eval_int_add_safe(t->data.integer, lhs, rhs, local);
    if (!ok) {
      Message_error(
        in->msg_sink,
        inst_location(f, pc),
        string_lit("int_add_safe overflow")
      );
      return Step_integer_overflow;
    }

    f->pc += 1;
  } break;

  case IIR_int_sub: {
    IIrBinary *bin = iir_chunk_extra(f->chunk, pc);
    void *lhs = resolve(in, f, bin->lhs);
    void *rhs = resolve(in, f, bin->rhs);

    Type *t = types_get(&in->compiler->types, type);
    b32 ok = eval_int_sub_safe(t->data.integer, lhs, rhs, local);
    if (!ok) {
      Message_error(
        in->msg_sink,
        inst_location(f, pc),
        string_lit("int_sub_safe overflow")
      );
      return Step_integer_overflow;
    }

    f->pc += 1;
  } break;

  case IIR_int_mul: {
    IIrBinary *bin = iir_chunk_extra(f->chunk, pc);
    void *lhs = resolve(in, f, bin->lhs);
    void *rhs = resolve(in, f, bin->rhs);

    Type *t = types_get(&in->compiler->types, type);
    b32 ok = eval_int_mul_safe(t->data.integer, lhs, rhs, local);
    if (!ok) {
      Message_error(
        in->msg_sink,
        inst_location(f, pc),
        string_lit("int_mul_safe overflow")
      );
      return Step_integer_overflow;
    }

    f->pc += 1;
  } break;

  case IIR_int_div: {
    IIrBinary *bin = iir_chunk_extra(f->chunk, pc);
    void *lhs = resolve(in, f, bin->lhs);
    void *rhs = resolve(in, f, bin->rhs);

    Type *t = types_get(&in->compiler->types, type);
    u32 err = eval_int_div_safe(t->data.integer, lhs, rhs, local);
    if (err) {
      if (err == IntOp_zero_division) {
        Message_error(
          in->msg_sink,
          inst_location(f, pc),
          string_lit("int_div_safe zero division")
        );

        return Step_zero_division;
      }

      if (err == IntOp_overflow) {
        Message_error(
          in->msg_sink,
          inst_location(f, pc),
          string_lit("int_div_safe overflow")
        );

        return Step_integer_overflow;
      }

      Unreachable();
    }

    f->pc += 1;
  } break;

  case IIR_int_mod: {
    IIrBinary *bin = iir_chunk_extra(f->chunk, pc);
    void *lhs = resolve(in, f, bin->lhs);
    void *rhs = resolve(in, f, bin->rhs);

    Type *t = types_get(&in->compiler->types, type);
    u32 err = eval_int_mod_safe(t->data.integer, lhs, rhs, local);
    if (err) {
      if (err == IntOp_zero_division) {
        Message_error(
          in->msg_sink,
          inst_location(f, pc),
          string_lit("int_mod_safe zero division")
        );

        return Step_zero_division;
      }

      Unreachable();
    }

    f->pc += 1;
  } break;

  case IIR_bit_not: {
    void *val = resolve(in, f, iir_chunk_ref(f->chunk, pc));
    *Cast(u8 *, local) = !*Cast(u8 *, val);
    f->pc += 1;
  } break;

  case IIR_bit_and: {
    IIrBinary *bin = iir_chunk_extra(f->chunk, pc);
    u8 *lhs = resolve(in, f, bin->lhs);
    u8 *rhs = resolve(in, f, bin->rhs);

    for (u32 i = 0; i < size_info.size; i++) {
      Cast(u8 *, local)[i] = lhs[i] & rhs[i];
    }

    f->pc += 1;
  } break;

  case IIR_bit_or: {
    Todo();
  } break;

  case IIR_bit_xor: {
    Todo();
  } break;

  case IIR_int_cmp_eq:
  case IIR_int_cmp_ne:
  case IIR_int_cmp_gt:
  case IIR_int_cmp_ge:
  case IIR_int_cmp_lt:
  case IIR_int_cmp_le: {
    IIrBinary *bin = iir_chunk_extra(f->chunk, pc);
    void *lhs = resolve(in, f, bin->lhs);
    void *rhs = resolve(in, f, bin->rhs);

    TypeInteger int_type = type_int_repr(&in->compiler->types, ref_type(in, f, bin->lhs));

    *Cast(u8 *, local) = eval_int_cmp(op, int_type, lhs, rhs) != 0;

    f->pc += 1;
  } break;

  }

  return Step_ok;
}

u32 interpreter_call(Interpreter* in, IIrChunk *chunk, void **args, u32 arg_count, void *out) {
  u32 depth = Cast(u32, in->call_stack.len);

  CallFrame2 *f = frame_push(in, chunk, out);

  for (u32 i = 0; i < arg_count; i++) {
    frame_set_param(in, f, i, args[i]);
  }

  while (True) {
    u32 err = step(in);

    // clang-format off
    switch (Cast(StepResult, err)) {
    case Step_ok: break;
    case Step_integer_overflow: return Interpret_integer_overflow;
    case Step_zero_division: return Interpret_zero_division;
    case Step_index_out_of_bounds: return Interpret_index_out_of_bounds;
    case Step_illegal_opcode: return Interpret_illegal_opcode;
    case Step_return: if (in->call_stack.len == depth) return Interpret_ok;
    }
    // clang-format on
  }

  Unreachable();
}
