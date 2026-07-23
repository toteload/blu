#include "interpret.h"
#include "ir.h"
#include "source_file.h"

extern u32 eval_coerce(TypeInterner *types, ValueStore *values, TypeIndex dst, Value *val, ValueIndex *res);

// The LiveValueStack is meant to keep track of live values, so that when you leave a scope or function
// these values can be freed. Otherwise, we would have an evergrowing heap.

enum LiveValueKind {
  LiveValue_marker_frame,
  LiveValue_marker_block,
  LiveValue_value,
};

typedef struct {
  u8 kind;
  InstructionIndex idx;
} LiveValueElement;

#define VALUESTACK_MIN_SIZE_LOG2  5
#define VALUESTACK_SEGMENT_COUNT  24
#define SEGMENTLIST_NAME          LiveValueStack
#define SEGMENTLIST_TYPE          LiveValueElement
#define SEGMENTLIST_MIN_SIZE_LOG2 VALUESTACK_MIN_SIZE_LOG2
#define SEGMENTLIST_SEGMENT_COUNT VALUESTACK_SEGMENT_COUNT
#define SEGMENTLIST_LINKAGE         internal
#define SEGMENTLIST_FUNCTION_PREFIX value_stack
#define SEGMENTLIST_OUTPUT_TYPES
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

internal u32 instruction_index_hash(void *context, InstructionIndex idx) {
  Unused(context);
  return idx;
}

internal b32 instruction_index_eq(void *context, InstructionIndex a, InstructionIndex b) {
  Unused(context);
  return a == b;
}

#define HASHMAP_NAME       InstValueMap
#define HASHMAP_KEY_TYPE   InstructionIndex
#define HASHMAP_VALUE_TYPE IrRef
#define HASHMAP_FUNCTION_PREFIX map
#define HASHMAP_HASH_FN         instruction_index_hash
#define HASHMAP_KEY_COMPARE_FN  instruction_index_eq
#define HASHMAP_LINKAGE         internal
#define HASHMAP_OUTPUT_TYPES
#define HASHMAP_OUTPUT_DEFINITIONS
#include "hashmap.h"

typedef struct {
  u32 ok;
  IrChunk *chunk;
  InstructionIndex pc;
  InstructionIndex end;
  InstValueMap inst_map;
} CallFrame;

#define CALLSTACK_MIN_SIZE_LOG2   5
#define CALLSTACK_SEGMENT_COUNT   24
#define SEGMENTLIST_NAME          CallStack
#define SEGMENTLIST_TYPE          CallFrame
#define SEGMENTLIST_FUNCTION_PREFIX call_stack
#define SEGMENTLIST_LINKAGE         internal
#define SEGMENTLIST_MIN_SIZE_LOG2 CALLSTACK_MIN_SIZE_LOG2
#define SEGMENTLIST_SEGMENT_COUNT CALLSTACK_SEGMENT_COUNT
#define SEGMENTLIST_OUTPUT_TYPES
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

typedef struct {
  IrBuilder builder;

  Arena *scratch;

  MessageSink *msg_sink;

  DeclarationInterner *declarations;
  TypeInterner *types;
  ValueStore *values;
  Common *common;

  IrRef result;

  LiveValueStack value_stack;
  CallStack      call_stack;
} Interpreter;

// The interpreter's per-frame maps grow with the C allocator; cstd_allocator is defined in compiler.c.
extern Allocator const cstd_allocator;

internal CallFrame *top_frame(Interpreter *in) {
  return call_stack_ptr_at_unchecked(&in->call_stack, in->call_stack.len - 1);
}

internal void store_inst_value(Interpreter *in, InstructionIndex idx, IrRef val) {
  map_insert(&top_frame(in)->inst_map, idx, val);
}

// A value-index ref is already a comptime value; an instruction-index ref is the result recorded
// for that instruction in the current frame.
internal IrRef resolve(CallFrame *f, IrRef ref) {
  if (ref_is_value_index(ref)) {
    return ref;
  }

  return *map_find(&f->inst_map, ref_to_instruction_index(ref));
}

// If this function returns False, then the ref refers to a residual value and has no comptime value.
internal b32 must_resolve(CallFrame *f, IrRef ref, ValueIndex *res) {
  if (ref_is_value_index(ref)) {
    *res = ref_to_value_index(ref);
    return True;
  }

  IrRef *x = map_find(&f->inst_map, ref_to_instruction_index(ref));

  Assert(!is_null(x)); // if the code is properly generated, this should never happen

  if (ref_is_instruction_index(*x)) {
    return False;
  }

  *res = ref_is_value_index(*x);

  return True;
}

internal void frame_push(Interpreter *in, IrChunk *chunk, InstructionIndex start, InstructionIndex end) {
  CallFrame *f = call_stack_push(&in->call_stack, in->scratch);
  f->chunk = chunk;
  f->pc    = start;
  f->end   = end;
  map_init(&f->inst_map, &(HashMapOptions){ .allocator = cstd_allocator, .initial_size = 8, .context = Null });

  value_stack_append(&in->value_stack, in->scratch, (LiveValueElement){ .kind = LiveValue_marker_frame });
}

// ASSUME: `v` refers to a TypeIndex
internal TypeIndex type_from_val(Interpreter *in, ValueIndex v) {
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

internal void step(Interpreter *in, CallFrame *f) {
  InstructionIndex pc = f->pc;
  u8 op = chunk_opcode(f->chunk, pc);

  switch (Cast(enum IrOpcode, op)) {
  case IR_block: {
    value_stack_append(&in->value_stack, in->scratch, (LiveValueElement){ .kind = LiveValue_marker_block, .idx = pc });
    f->pc = pc + 1;
  } break;
  case IR_lookup: {
    DeclarationIndex decl_idx = chunk_data(f->chunk, pc);
    Declaration decl = decls_get_extra(in->declarations, decl_idx);

    Assert(decl.kind == Declaration_value); // only values supported for now

    store_inst_value(in, pc, ir_ref_from_value_index(decl.data.val));
    f->pc = pc + 1;
  } break;
  case IR_as: {
    IrAs *as = chunk_extra(f->chunk, pc);
    IrRef ref = resolve(f, as->val);

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
    } else {
      // as->val is a non-comptime known value, so we can only check if the types are valid for coercion.
      // probably also insert some sort of widening cast that cannot fail
      Panic(); // TODO
    }

    Panic(); // TODO save the inst value
    f->pc = pc + 1;
  } break;
  case IR_br: {
    IrBr *br = chunk_extra(f->chunk, pc);
    IrRef val = resolve(f, br->value);
    store_inst_value(in, br->block, val);
    in->result = val;
    f->pc = br->block + chunk_data(f->chunk, br->block) + 1;
  } break;
  case IR_type: {
    IrType *type = chunk_extra(f->chunk, pc);
    TypeIndex t;
    switch (Cast(enum TypeKind, type->kind)) {
    case Type_function: {
      Assert(type->arg_count >= 1);

      Assert(type->arg_count == 1); // For now don't support params
      TypeIndex return_type = type_from_val(in, type->args[0]);
      t = types_add(in->types, &(Type){
        .kind = Type_function,
        .data.function = { .return_type = return_type, .param_count = 0 },
      });
      
    } break;
    default: Panic();
    }
    ValueIndex v = val_from_type(in, t);
    store_inst_value(in, pc, v);
    f->pc += 1;
  } break;
  default: Panic();
  }
}

// Run pushed frames until the call stack returns to `base_depth`; the finished region's value is in
// `in->result`.
internal IrRef run(Interpreter *in, u32 base_depth) {
  while (in->call_stack.len > base_depth) {
    CallFrame *f = top_frame(in);
    if (f->pc >= f->end) {
      in->call_stack.len -= 1;
      // ponytail: no inter-frame result routing yet; add when IR_call/IR_ret land.
      continue;
    }
    step(in, f);
  }
  return in->result;
}

// Evaluate a self-contained region (e.g. a declaration's declared-type block) on a fresh frame.
internal IrRef eval_region(Interpreter *in, IrChunk *chunk, InstructionIndex start, InstructionIndex end) {
  u32 base = in->call_stack.len;
  frame_push(in, chunk, start, end);
  return run(in, base);
}

b32 source_interpret_declaration(InterpretContext *context, Source *source, u32 idx_declaration) {
  IrChunk *chunk = &source->ir_chunks[idx_declaration];

  Assert(chunk->opcodes[0] == IR_declaration);

  IrDeclaration *decl = chunk_extra(chunk, 0);

  Interpreter in = {
    .scratch      = context->scratch,
    .declarations = context->decls,
  };

  if (!ir_ref_is_nil(decl->declared_type)) {
    InstructionIndex block = ref_to_instruction_index(decl->declared_type);
    InstructionIndex end   = block + chunk_data(chunk, block) + 1;
    IrRef declared_type = eval_region(&in, chunk, block, end);
    Unused(declared_type); // TODO: check/coerce decl->value against this
    Panic();
  }

  // TODO: evaluate decl->value and emit residual IR. For now emit an empty runtime chunk so the
  // pipeline's print stage has something well-formed to walk.
  irbuilder_flatten(&in.builder, context->perm, &source->runtime_chunks[idx_declaration]);

  return True;
}
