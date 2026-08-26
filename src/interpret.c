#include "interpret.h"
#include "value.h"
#include "eval.h"
#include "print.h"

#define MAX_SCOPE_DEPTH 64

internal void *resolve(Interpreter *in, CallFrame2 *f, IRef ref) {
  if (iref_is_value(ref)) {
    Value *v = values_get(&in->compiler->values, iref_to_value(ref));
    return v->data;
  } else {
    return f->inst_values[iref_to_instruction(ref)];
  }
}

internal CallFrame2 *frame_push(Interpreter *in, IIrChunk *chunk, void *ret) {
  CallFrame2 *f = stack_push_ptr(&in->call_stack);

  u32 count = chunk->opcode_count;

  f->ret = ret;
  f->chunk = chunk;
  f->inst_values = arena_push_array(void*, in->scratch, count);

  stack_init(&f->scope_stack, arena_push_array(ScopeSpan2, in->scratch, MAX_SCOPE_DEPTH), MAX_SCOPE_DEPTH);
  stack_push(&f->scope_stack, ((ScopeSpan2){ .start = 0, .end = count, .snapshot = arena_scope_begin(in->scratch) }));

  for (u32 i = 1; i < count; i++) {
    if (iir_chunk_op(chunk, i) != IIR_param) {
      f->pc = i;
      break;
    }
  }

  return f;
}

internal void frame_pop(Interpreter *in) {
  CallFrame2 *f = stack_peek_ptr(&in->call_stack);

  ScopeSpan2 *span = f->scope_stack.data;
  arena_scope_end(in->scratch, span->snapshot);

  stack_pop_unchecked(&in->call_stack);
}

internal void scopes_pop_to(Interpreter *in, CallFrame2 *f, InstructionIndex block) {
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
  Step_illegal_opcode,
} StepResult;

internal u32 step(Interpreter *in) {
  CallFrame2 *f = stack_peek_ptr(&in->call_stack);
  InstructionIndex pc = f->pc;
  IIrOpcode op = f->chunk->opcodes[pc];

  void *local;
  TypeSizeInfo size_info;
  TypeIndex type = iir_chunk_type(f->chunk, pc);
  if (type) {
    size_info = types_size_info_by_index(&in->compiler->types, type);
    local = arena_push(in->scratch, size_info.size, size_info.align);
    f->inst_values[pc] = local;
  }

  switch (op) {
  case IIR_func: {
    Message_error(
      in->msg_sink,
      (MessageLocation){ .kind = MessageLocation_unspecified, },
      string_lit("Stepped into IIR_func instruction which is only allowed to be called")
    );
    return Step_illegal_opcode;
  } break;

  case IIR_param: {
    Message_error(
      in->msg_sink,
      (MessageLocation){ .kind = MessageLocation_unspecified, },
      string_lit("Stepped into IIR_param instruction which should be set by caller")
    );
    return Step_illegal_opcode;
  } break;

  case IIR_loop: { Todo(); } break;
  case IIR_repeat: { Todo(); } break;

  case IIR_block: {
    u32 inst_count = iir_chunk_data(f->chunk, pc);
    stack_push(&f->scope_stack, ((ScopeSpan2){ .start = pc, .end = pc + inst_count, .snapshot = arena_scope_begin(in->scratch) }));
    f->pc += 1;
  } break;

  case IIR_br: {
    IIrBr *br = iir_chunk_extra(f->chunk, pc);
    void *src = resolve(in, f, br->value);
    memcpy(f->inst_values[br->block], src, size_info.size);
    scopes_pop_to(in, f, br->block);
    u32 inst_count = iir_chunk_data(f->chunk, br->block);
    f->pc = br->block + inst_count;
  } break;

  case IIR_condbr: {
    IIrCondbr *condbr = iir_chunk_extra(f->chunk, pc);
    u8* v = resolve(in, f, condbr->cond);
    if (*v) {
      f->pc = condbr->then;
    } else {
      f->pc = condbr->otherwise;
    }
  } break;

  case IIR_call: {
    IIrCall *call = iir_chunk_extra(f->chunk, pc);
    ValueFunc *func = resolve(in, f, call->func_ptr);

    IIrChunk *chunk = &func->chunk;

    CallFrame2 *g = frame_push(in, chunk, f->inst_values[pc]);

    for (u32 i = 0; i < call->arg_count; i++) {
      TypeSizeInfo param_size_info =
        types_size_info_by_index(&in->compiler->types, iir_chunk_type(chunk, 1 + i));

      void *slot = arena_push(in->scratch, param_size_info.size, param_size_info.align);
      memcpy(slot, resolve(in, f, call->args[i]), param_size_info.size);

      g->inst_values[1 + i] = slot;
    }

    f->pc += 1;
  } break;

  case IIR_ret: {
    void *src = resolve(in, f, (IRef){iir_chunk_data(f->chunk, pc)});
    memcpy(f->ret, src, size_info.size);
    frame_pop(in);
    return Step_return;
  } break;

  case IIR_builtin_debug: {
    void *p = resolve(in, f, (IRef){iir_chunk_data(f->chunk, pc)});
    memcpy(f->inst_values[pc], p, size_info.size);
    print_value_raw(stdout, in->compiler, 0, type, p);
    fputs("\n", stdout);
    f->pc += 1;
  } break;

  case IIR_alloc: {
    void *alloc = arena_push(in->scratch, size_info.size, size_info.align);
    void **p = arena_push_one(void*, in->scratch);
    *p = alloc;
    f->inst_values[pc] = p;
    f->pc += 1;
  } break;

  case IIR_store: {
    IIrStore *store = iir_chunk_extra(f->chunk, pc);
    void **p = resolve(in, f, store->ptr);
    void *val = resolve(in, f, store->value);
    memcpy(*p, val, size_info.size);
    f->pc += 1;
  } break;

  case IIR_load: {
    IRef ref = (IRef){ iir_chunk_data(f->chunk, pc) };
    void **p = resolve(in, f, ref);
    memcpy(f->inst_values[pc], *p, size_info.size);
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
        (MessageLocation){ .kind = MessageLocation_unspecified, },
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
        (MessageLocation){ .kind = MessageLocation_unspecified, },
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
        (MessageLocation){ .kind = MessageLocation_unspecified, },
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
          (MessageLocation){ .kind = MessageLocation_unspecified, },
          string_lit("int_div_safe zero division")
        );

        return Step_zero_division;
      }

      if (err == IntDivSafe_overflow) {
        Message_error(
          in->msg_sink,
          (MessageLocation){ .kind = MessageLocation_unspecified, },
          string_lit("int_div_safe overflow")
        );

        return Step_integer_overflow;
      }

      Unreachable();
    }

    f->pc += 1;
  } break;

  case IIR_int_mod: {
    Todo();
  } break;

  case IIR_bit_and: {
    Todo();
  } break;

  case IIR_bit_or: {
    Todo();
  } break;

  case IIR_bit_xor: {
    Todo();
  } break;
  }

  return Step_ok;
}

u32 interpreter_call(Interpreter* in, IIrChunk *chunk, ValueIndex *args, u32 arg_count, void *out) {
  // TODO: make sure that the args match the signature of the function
  CallFrame2 *f = frame_push(in, chunk, out);

  for (u32 i = 0; i < arg_count; i++) {
    Todo();
  }

  while (True) {
    u32 err = step(in);

    // clang-format off
    switch (Cast(StepResult, err)) {
    case Step_ok: break;
    case Step_integer_overflow: return Interpret_integer_overflow;
    case Step_zero_division: return Interpret_zero_division;
    case Step_illegal_opcode: return Interpret_illegal_opcode;
    case Step_return: if (in->call_stack.len == 0) return 0;
    }
    // clang-format on
  }

  Unreachable();
}
