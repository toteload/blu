#include "interpreter.h"
#include "value.h"

#define MAX_SCOPE_DEPTH 64

internal ValueIndex resolve(Interpreter2 *in, CallFrame2 *f, IrRef ref) {
  ValueIndex val = 0;

  if (ref_is_some_value_index(ref)) {
    val = ref_to_value_index(ref);
  }

  if (ref_is_instruction_index(ref)) {
    val = f->inst_values[ref_to_instruction_index(ref)];
  }

  if (val) {
    return values_copy(in->values, val);
  }

  return 0;
}

internal CallFrame2 *frame_push(Interpreter2 *in, IrChunk *chunk, ValueIndex *ret) {
  CallFrame2 *f = stack_push_ptr(&in->call_stack);

  u32 count = chunk->opcode_count;

  f->ret = ret;
  f->chunk = chunk;
  f->snapshot = arena_scope_begin(in->scratch);
  f->inst_values = arena_push_array(ValueIndex, in->scratch, count);

  stack_init(&f->scope_stack, arena_push_array(ScopeSpan2, in->scratch, MAX_SCOPE_DEPTH), MAX_SCOPE_DEPTH);

  for (u32 i = 1; i < count; i++) {
    if (chunk_opcode(chunk, i) != IR_param) {
      f->pc = i;
      break;
    }
  }

  return f;
}

internal void frame_pop(Interpreter2 *in) {
  CallFrame2 *f = stack_peek_ptr(&in->call_stack);

  for (u32 i = 1; i < f->chunk->opcode_count; i++) {
    if (f->inst_values[i]) {
      values_dealloc(in->values, f->inst_values[i]);
    }
  }

  arena_scope_end(in->scratch, f->snapshot);

  stack_pop_unchecked(&in->call_stack);
}

internal void scopes_pop_to(Interpreter2 *in, CallFrame2 *f, InstructionIndex block) {
  while (True) {
    ScopeSpan2 span = stack_pop(&f->scope_stack);

    if (span.start == block) {
      // Do not dealloc the value stored at the block address
      for (u32 i = span.start + 1; i < span.end; i++) {
        if (f->inst_values[i]) {
          values_dealloc(in->values, f->inst_values[i]);
          f->inst_values[i] = 0;
        }
      }

      return;
    }
  }

  Unreachable();
}

typedef enum {
  Step_ok,
  Step_return,
} StepResult;

internal u32 step(Interpreter2 *in) {
  CallFrame2 *f = stack_peek_ptr(&in->call_stack);

  InstructionIndex pc = f->pc;

  u8 op = chunk_opcode(f->chunk, pc);
  switch (Cast(IrOpcode, op)) {
  case IR_block: {
    u32 inst_count = chunk_data(f->chunk, pc);
    stack_push(&f->scope_stack, ((ScopeSpan2){ .start = pc, .end = pc + inst_count }));
    f->pc += 1;
  } break;
  case IR_br: {
    IrBr *br = chunk_extra(f->chunk, pc);
    f->inst_values[br->block] = resolve(in, f, br->value);
    scopes_pop_to(in, f, br->block);
    u32 inst_count = chunk_data(f->chunk, br->block);
    f->pc = br->block + inst_count;
  } break;
  case IR_condbr: {
    IrCondBr *condbr = chunk_extra(f->chunk, pc);
    Value *v = values_get(in->values, resolve(in, f, condbr->cond));
    if (*Cast(u8*,v->data)) {
      f->pc = condbr->then;
    } else {
      f->pc = condbr->otherwise;
    }
  } break;
  case IR_call: {
    IrCall *call = chunk_extra(f->chunk, pc);
    Value *v = values_get(in->values, resolve(in, f, call->func));
    IrChunk *chunk = &Cast(ValueFunc*, v->data)->chunk;
    CallFrame2 *g = frame_push(in, chunk, &f->inst_values[pc]);

    for (u32 i = 0; i < call->arg_count; i++) {
      g->inst_values[i+1] = resolve(in, f, call->args[i]);
    }

    f->pc += 1;
  } break;
  case IR_ret: {
    *f->ret = resolve(in, f, (IrRef){ chunk_data(f->chunk, pc) });
    frame_pop(in);
    return Step_return;
  } break;
  case IR_builtin_debug: {
    ValueIndex val = resolve(in, f, (IrRef){ chunk_data(f->chunk, pc) });
    value_print(stdout, in->types, in->values, val);
    fprintf(stdout, "\n");
    f->inst_values[pc] = val;
    f->pc += 1;
  } break;
  default: Todo();
  }

  return Step_ok;
}

u32 interpreter_call(Interpreter2* in, IrChunk *chunk, ValueIndex *args, u32 arg_count, ValueIndex *out) {
  // TODO: make sure that the args match the signature of the function
  CallFrame2 *f = frame_push(in, chunk, out);

  for (u32 i = 0; i < arg_count; i++) {
    Todo();
  }

  while (True) {
    u32 err = step(in);

    if (err == Step_ok) {
      continue;
    }

    if (err == Step_return) {
      if (in->call_stack.len == 0) {
        return 0;
      }
    }
  }

  Unreachable();
}
