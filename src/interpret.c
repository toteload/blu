#include "interpret.h"
#include "eval.h"
#include "print.h"

#define MAX_SCOPE_DEPTH 64

typedef struct {
  usize len;
  void *data;
} RawSlice;

internal TypeSizeInfo layout_of(Interpreter *in, TypeKey type) {
  if (key_is_nil(type)) {
    return (TypeSizeInfo){ .size = 0, .stride = 0, .align = 1 };
  }

  TypeSizeInfo info = types_size_info_by_key(&in->compiler->types, type);
  info.align = Max(info.align, 1);
  return info;
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

internal void *constant_data(Interpreter *in, ValueKey key);

internal void materialize(Interpreter *in, TypeKey type, void const *src, void *dst) {
  TypeInterner *types = &in->compiler->types;
  Type *t = types_get(types, type);

  switch (Cast(TypeKind, t->kind)) {
  case Type_integer:
  case Type_bool:
  case Type_type:
  case Type_nil:
  case Type_never:
  case Type_function: {
    TypeSizeInfo size_info = types_size_info(types, t);
    if (size_info.size > 0) {
      memcpy(dst, src, size_info.size);
    }
  } break;

  case Type_array: {
    TypeSizeInfo elem = types_size_info_by_key(types, t->data.array.base_type);
    for (u64 i = 0; i < t->data.array.size; i++) {
      materialize(
        in,
        t->data.array.base_type,
        ptr_offset(src, i * elem.stride),
        ptr_offset(dst, i * elem.stride)
      );
    }
  } break;

  case Type_slice: {
    ValueSlice const *slice = src;
    void *pointee = constant_data(in, slice->ptr.val);
    *Cast(RawSlice *, dst) = (RawSlice){
      .len = slice->len,
      .data = ptr_offset(pointee, slice->ptr.offset),
    };
  } break;

  case Type_pointer: {
    ValuePointer const *p = src;
    void *pointee = constant_data(in, p->val);
    *Cast(void **, dst) = ptr_offset(pointee, p->offset);
  } break;

  case Type_comptime_int:
  case Type_stub:
    Unreachable();
  }
}

internal void *constant_data(Interpreter *in, ValueKey key) {
  key = follow_stubs(in->compiler, key);

  void **slot = &in->constants[key.x];
  if (*slot) {
    return *slot;
  }

  Value *v = values_get(&in->compiler->values, key);
  TypeSizeInfo info = layout_of(in, v->type);
  void *data = arena_push(in->perm, info.size, info.align);
  *slot = data;
  materialize(in, v->type, v->data, data);

  return data;
}

internal u32 slot_alloc(Interpreter *in, TypeSizeInfo info) {
  void *p = arena_push(in->scratch, info.size, info.align);
  return Cast(u32, ptr_diff(p, in->scratch->base));
}

internal void *slot_ptr(Interpreter *in, CallFrame2 *f, InstructionOffset inst) {
  return ptr_offset(in->scratch->base, f->value_offsets[inst]);
}

internal void *resolve(Interpreter *in, CallFrame2 *f, IRef ref) {
  if (iref_is_value(ref)) {
    return constant_data(in, iref_to_value(ref));
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
  ArenaSnapshot snapshot = arena_scope_begin(in->scratch);

  CallFrame2 *f = stack_push_ptr(&in->call_stack);

  u32 count = chunk->opcode_count;

  f->ret = ret;
  f->chunk = chunk;
  f->value_offsets = arena_push_array(u32, in->scratch, count);

  stack_init(&f->scope_stack, arena_push_array(ScopeSpan2, in->scratch, MAX_SCOPE_DEPTH), MAX_SCOPE_DEPTH);
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
  f->value_offsets[inst] = slot_alloc(in, size_info);
  copy_value(slot_ptr(in, f, inst), src, size_info.size);
}

internal void frame_pop(Interpreter *in) {
  CallFrame2 *f = stack_peek_ptr(&in->call_stack);

  ScopeSpan2 *span = f->scope_stack.data;
  arena_scope_end(in->scratch, span->snapshot);

  stack_pop_unchecked(&in->call_stack);
}

internal void scope_push(Interpreter *in, CallFrame2 *f, InstructionOffset block) {
  u32 inst_count = iir_chunk_data(f->chunk, block);
  stack_push(&f->scope_stack, ((ScopeSpan2){
    .start = block,
    .end = block + inst_count,
    .snapshot = arena_scope_begin(in->scratch),
  }));
}

internal void scopes_pop_to(Interpreter *in, CallFrame2 *f, InstructionOffset block) {
  while (True) {
    ScopeSpan2 span = stack_pop(&f->scope_stack);

    if (span.start == block) {
      arena_scope_end(in->scratch, span.snapshot);
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
  AstAndSourceIndex src = f->chunk->sources[pc];
  if (!src.ast_idx) {
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

  f->value_offsets[pc] = slot_alloc(in, size_info);
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
      RawSlice *slice = resolve(in, f, bin->lhs);
      base = slice->data;
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

    u32 stride = types_size_info_by_key(&in->compiler->types, type).stride;
    copy_value(local, ptr_offset(base, at * stride), size_info.size);

    f->pc += 1;
  } break;

  case IIR_builtin_len: {
    RawSlice *slice = resolve(in, f, iir_chunk_ref(f->chunk, pc));
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
    TypeSizeInfo base = types_size_info_by_key(&in->compiler->types, t->data.pointer.base_type);
    *Cast(void **, local) = arena_push(in->scratch, base.size, Max(base.align, 1));
    f->pc += 1;
  } break;

  case IIR_store: {
    IIrStore *store = iir_chunk_extra(f->chunk, pc);
    void **p = resolve(in, f, store->ptr);
    void *val = resolve(in, f, store->value);
    copy_value(*p, val, size_info.size);
    f->pc += 1;
  } break;

  case IIR_load: {
    IRef ref = iir_chunk_ref(f->chunk, pc);
    void **p = resolve(in, f, ref);
    copy_value(local, *p, size_info.size);
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
      if (err == IntDivSafe_zero_division) {
        Message_error(
          in->msg_sink,
          inst_location(f, pc),
          string_lit("int_div_safe zero division")
        );

        return Step_zero_division;
      }

      if (err == IntDivSafe_overflow) {
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
      if (err == IntDivSafe_zero_division) {
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

    Type *t = types_get(&in->compiler->types, ref_type(in, f, bin->lhs));

    i32 ord;
    if (t->data.integer.signedness == Signed) {
      i64 l = read_int_sign_extend(t->data.integer.bitwidth, lhs);
      i64 r = read_int_sign_extend(t->data.integer.bitwidth, rhs);
      ord = (l > r) - (l < r);
    } else {
      u64 l = read_int_zero_extend(t->data.integer.bitwidth, lhs);
      u64 r = read_int_zero_extend(t->data.integer.bitwidth, rhs);
      ord = (l > r) - (l < r);
    }

    b32 res;
    // clang-format off
    switch (op) {
    case IIR_int_cmp_eq: res = ord == 0; break;
    case IIR_int_cmp_ne: res = ord != 0; break;
    case IIR_int_cmp_gt: res = ord >  0; break;
    case IIR_int_cmp_ge: res = ord >= 0; break;
    case IIR_int_cmp_lt: res = ord <  0; break;
    case IIR_int_cmp_le: res = ord <= 0; break;
    default: Unreachable();
    }
    // clang-format on

    *Cast(u8 *, local) = res != 0;

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
