#include "interpret.h"
#include "ir.h"
#include "source_file.h"
#include "eval.h"

// For simplicity the maximum depths are a fixed number. This will likely change.
#define MAX_SCOPE_DEPTH 64
#define MAX_CALL_DEPTH  128

typedef struct {
  InstructionIndex start;
  InstructionIndex end;
} ScopeSpan;

typedef Stack(ScopeSpan) ScopeStack;

typedef struct {
  // This struct probably will also have to carry arguments for when function calls get added.
  // And return value (maybe?)

  b32 ok;

  InstructionIndex pc;

  IrChunk *chunk;

  ArenaSnapshot snapshot;

  ValueIndex *inst_map;

  ScopeStack scopes;
} CallFrame;

typedef Stack(CallFrame) CallStack;

typedef struct {
  IrBuilder builder;

  CallStack call_stack;

  Arena *scratch;
  MessageSink *msg_sink;
  DeclarationInterner *declarations;
  TypeInterner *types;
  ValueStore *values;
  Common *common;
} Interpreter;

// The interpreter's per-frame maps grow with the C allocator; cstd_allocator is defined in compiler.c.
extern Allocator const cstd_allocator;

internal always_inline CallFrame *top_frame(Interpreter *in) {
  return stack_peek_ptr_unsafe(&in->call_stack);
}

internal always_inline void store_inst_value(CallFrame *f, InstructionIndex idx, IrRef val) {
  f->inst_map[idx] = val;
}

void frame_push(Interpreter *in, IrChunk *chunk) {
  CallFrame f = {
    .chunk = chunk,
    .pc = 0,
    .snapshot = arena_scope_begin(in->scratch),
    .inst_map = arena_push_array(ValueIndex, in->scratch, chunk->opcode_count),
  };

  stack_init(&f.scopes, arena_push_array(ScopeSpan, in->scratch, MAX_SCOPE_DEPTH), MAX_SCOPE_DEPTH);
  stack_push(&f.scopes, ((ScopeSpan){ .start = start, .end = chunk->opcode_count }));

  stack_push(&in->call_stack, f);
}

void frame_pop(Interpreter *in) {
  CallFrame f = stack_pop_unsafe(&in->call_stack);

  ScopeSpan span = f.scopes.data[0];
  for (u32 i = span.start; i < span.end; i++) {
    if (f.inst_map[i]) {
      values_dealloc(in->values, f.inst_map[i]);
    }
  }

  arena_scope_end(in->scratch, f.snapshot);
}

internal void scope_push(CallFrame *f, u32 inst_count) {
  stack_push(&f->scopes, ((ScopeSpan){ .start = f->pc, .end = f->pc + inst_count }));
}

internal void scopes_end(Interpreter *in, CallFrame *f, InstructionIndex idx) {
  for (u32 i = f->scopes.len; i-- > 0;) {
    ScopeSpan span = f->scopes.data[i];
    if (span.start == idx) {
      // Do not dealloc the value stored at the block address
      for (u32 j = span.start + 1; j < span.end; j++) {
        if (f->inst_map[j]) {
          values_dealloc(in->values, f->inst_map[j]);
          f->inst_map[j] = 0;
        }
      }

      break;
    }
  }
}

// A value-index ref is already a comptime value; an instruction-index ref is the result recorded
// for that instruction in the current frame.
internal IrRef resolve(CallFrame *f, IrRef ref) {
  if (ref_is_value_index(ref)) {
    return ref;
  }

  return f->inst_map[ref_to_instruction_index(ref)];
}

// If this function returns False, then the ref refers to a residual value and has no comptime value.
internal b32 must_resolve(CallFrame *f, IrRef ref, ValueIndex *res) {
  if (ref_is_value_index(ref)) {
    *res = ref_to_value_index(ref);
    return True;
  }

  IrRef x = f->inst_map[ref_to_instruction_index(ref)];

  Assert(!ir_ref_is_nil(x)); // if the code is properly generated, this should never happen

  if (ref_is_instruction_index(x)) {
    return False;
  }

  *res = ref_to_value_index(x);

  return True;
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

void step(Interpreter *in, CallFrame *f) {
  InstructionIndex pc = f->pc;
  u8 op = chunk_opcode(f->chunk, pc);

  switch (Cast(enum IrOpcode, op)) {
  case IR_block: {
    u32 inst_count = chunk_data(f->chunk, pc);
    scope_push(f, inst_count);
    f->pc = pc + 1;
  } break;
  case IR_func: {
    IrFunc *func = chunk_extra(f->chunk, pc);
    u32 inst_count = func->instruction_count;
    scope_push(f, inst_count);
    f->pc += 1;
  } break;
  case IR_lookup: {
    DeclarationIndex decl_idx = chunk_data(f->chunk, pc);
    Declaration *decl = decls_extra_get_ptr(in->declarations, decl_idx);

    if (decl->kind == Declaration_decl) {
      Todo();
      decl_resolve_full();
    }

    Assert(decl.kind == Declaration_value); // only values supported for now

    ValueIndex copy = values_copy(in->values, decl.data.val);
    store_inst_value(f, pc, ir_ref_from_value_index(copy));
    f->pc = pc + 1;
  } break;
  case IR_as: {
    IrAs *as = chunk_extra(f->chunk, pc);
    IrRef ref = resolve(f, as->val);

    IrRef res;

    // If the value is comptime known, we try to do the coercion right away.
    if (ref_is_value_index(ref)) {
      Value *v = values_get(in->values, ref_to_value_index(ref));

      // TODO: may we assume that `type_to` is always comptime known?
      ValueIndex idx_dst_type;
      b32 ok = must_resolve(f, as->type_to, &idx_dst_type); 
      if (!ok) {
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
    scopes_end(in, f, br->block);
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

    ValueIndex lhs;
    b32 ok = must_resolve(f, unify->type_lhs, &lhs);
    Assert(ok);

    ValueIndex rhs;
    ok = must_resolve(f, unify->type_rhs, &rhs);
    Assert(ok);

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

    ValueIndex func;
    b32 ok = must_resolve(f, ref_func, &func);

    TypeIndex idx = type_from_val(in, func);
    Type *t = types_get(in->types, idx);
    ValueIndex v = val_from_type(in, t->data.function.return_type);
    store_inst_value(f, pc, v);
    f->pc += 1;
  } break;
  case IR_ret: {
    IrBuilder *builder = &in->builder;
    InstructionIndex inst = inst_alloc(builder);
    inst_set_opcode(builder, inst, IR_ret);
    inst_set_data(builder, inst, chunk_data(f->chunk, pc));
    f->pc += 1;
  } break;
  default: Panic();
  }
}

internal IrRef eval_block(Interpreter *in, IrChunk *chunk, InstructionIndex block) {
  Assert(chunk_opcode(chunk, block) == IR_block);

  CallFrame *base = top_frame(in);
  base->pc = block;

  InstructionIndex end = block + chunk_data(chunk, block);
  while (True) {
    CallFrame *f = top_frame(in);
    if (f == base && f->pc == end) {
      return f->inst_map[block];
    }
    step(in, f);
  }

  Unreachable();
}

internal IrRef eval_func(Interpreter *in, IrChunk *chunk, InstructionIndex idx) {
  Assert(chunk_opcode(chunk, idx) == IR_func);

  CallFrame *base = top_frame(in);
  base->pc = idx;

  IrFunc *func = chunk_extra(chunk, idx);
  InstructionIndex end = idx + func->instruction_count;

  while (True) {
    CallFrame *f = top_frame(in);
    if (f == base && f->pc == end) {
      return f->inst_map[idx];
    }
    step(in, f);
  }

  Unreachable();
}

// How to generate code for a function:
// 1. Evaluate the type expression for the declared type. This may not reference the function itself.
// 2. Evaluate the "prototype" of the function value. Make sure the prototype and the declared type can be unified.
// 3. Output a function for the runtime code.

b32 source_interpret_declaration(InterpretContext *context, Source *source, u32 idx_declaration) {
  IrChunk *chunk = &source->ir_chunks[idx_declaration];

  Assert(chunk->opcodes[0] == IR_declaration);

  IrDeclaration *decl = chunk_extra(chunk, 0);

  Interpreter in = {
    .scratch      = context->scratch,
    .declarations = context->decls,
    .types        = context->types,
    .values       = context->values,
    .common       = context->common,
    .builder      = { .scratch = context->scratch },
  };

  stack_init(&in.call_stack, arena_push_array(CallFrame, context->scratch, MAX_CALL_DEPTH), MAX_CALL_DEPTH);
  frame_push(&in, chunk, 0, chunk->opcode_count);

  IrRef declared_type = 0;
  if (!ir_ref_is_nil(decl->declared_type)) {
    Assert(ref_is_instruction_index(decl->declared_type));
    declared_type = eval_block(&in, chunk, ref_to_instruction_index(decl->declared_type));
  }
  Assert(ref_is_value_index(declared_type));

  Assert(ref_is_instruction_index(decl->value));
  if (chunk_opcode(chunk, ref_to_instruction_index(decl->value)) == IR_func) {
    // TODO func_type and declared_type must unify and be complete
    IrRef decl_value = eval_func(&in, chunk, ref_to_instruction_index(decl->value));
  } else {
    Panic();
  }

  irbuilder_flatten(&in.builder, context->perm, &source->runtime_chunks[idx_declaration]);

  frame_pop(&in);

  return True;
}

b32 resolve_decl_full(..., Declaration *decl) {
}

CallFrame *alloc_callframe(IrChunk *chunk) {
  Todo();
}

typedef struct {
  Declaration *decl;
  CallFrame   *frame;
} DeclarationResolution;

b32 resolve_declarations() {

  // This should turn into a stack of call stack, but I don't support function calls yet, so one frame
  // per resolve suffices.
  Stack(DeclarationResolution) resolve_stack;

  for (u32 i = 0; i < compiler->decls.len; i++) {
    Declaration *decl = decls_ptr_at_unchecked(&compiler->decls, i);
    CallFrame *f = alloc_callframe({}, ..., decl->data.decl.chunk);
    stack_push(&resolve_stack, { .decl = decl, .frame = f });
  }

  while (!stack_is_empty(&resolve_stack)) {
    DeclarationResolution resolution = stack_pop_unsafe(&resolve_stack);

    CallFrame *f = resolution.frame;
    u8 resolve_status = resolution.decl->resolve_status;

    // TODO: in some cases it is good enough if only the type of a declaration has been resolved
    // and not yet the value. For example, a recursive call or a type that contains a pointer to
    // itself. In the second case it must be through a pointer (an indirection) try to contain
    // itself directly would be illegal.

    if (resolve_status == ResolveStatus_fully_resolved || resolve_status == ResolveStatus_resolving_value) {
      continue;
    }

    if (resolve_status == ResolveStatus_resolving_type) {
      Panic();
    }

    if (resolve_status == ResolveStatus_unresolved) {
      resolution.decl->resolve_status = ResolveStatus_resolving_type;

      u32 err = run_until({}, f, resolution.decl->data.decl.typecheck_end);
      if (err == Run_resolve_declaration) {
        DeclarationIndex idx = chunk_data(f->chunk, f->pc);

        Todo();
      }

      resolution.decl->resolve_status = ResolveStatus_type_resolved;
    }

    if (resolve_status == ResolveStatus_type_resolved) {
      resolution.decl->resolve_status = ResolveStatus_resolving_value;
    }

  }
}
