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

    CallFrame2 *g = frame_push(in, chunk, &f->inst_values[pc]);

    for (u32 i = 0; i < call->arg_count; i++) {
      Todo();
    }

    //for (u32 i = 0; i < call->arg_count; i++) {
    //  g->inst_values[i+1] = values_copy(&in->compiler->values, resolve(in, f, call->args[i]));
    //}

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
    eval_int_add(t->data.integer, lhs, rhs, local);

    f->pc += 1;
  } break;

  default: Todo();
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
