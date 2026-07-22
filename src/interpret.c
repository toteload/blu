#include "interpret.h"
#include "ir.h"
#include "source_file.h"

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
#define HASHMAP_VALUE_TYPE ValueIndex
#define HASHMAP_FUNCTION_PREFIX map
#define HASHMAP_HASH_FN         instruction_index_hash
#define HASHMAP_KEY_COMPARE_FN  instruction_index_eq
#define HASHMAP_LINKAGE         internal
#define HASHMAP_OUTPUT_TYPES
#define HASHMAP_OUTPUT_DEFINITIONS
#include "hashmap.h"

typedef struct {
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
  IrChunk   *chunk;
  IrBuilder  builder;

  Arena *scratch;

  IrRef result;

  LiveValueStack value_stack;
  CallStack      call_stack;
} Interpreter;

internal void store_inst_value(Interpreter *interpreter, InstructionIndex idx, IrRef val) {
  Panic();
}

internal b32 run(Interpreter *interpreter, InstructionIndex idx) {
  u8 opcode = chunk_opcode(interpreter->chunk, idx);

  switch (opcode) {
  case IR_block: {
    value_stack_append(&interpreter->value_stack, interpreter->scratch, (LiveValueElement){ .kind = LiveValue_marker_block });

  } break;
  case IR_lookup: {
    DeclarationIndex decl_idx = chunk_data(interpreter->chunk, idx);
    Declaration decl = decls_get(interpreter->declarations, decl_idx);

    Assert(decl.kind == Declaration_value); // Only support values for now

    store_inst_value(interpreter, idx, decl.data.val);
  } break;
  case IR_as: {} break;
  case IR_unify: {} break;
  case IR_type: {} break;
  case IR_function_return_type: {} break;
  }
}

b32 source_interpret_declaration(InterpretContext *context, Source *source, u32 idx_declaration) {
  IrChunk *chunk = &source->ir_chunks[idx_declaration];

  Assert(chunk->opcodes[0] == IR_declaration);

  IrDeclaration *decl = chunk_extra(chunk, 0);
  if (!ir_ref_is_nil(decl->declared_type)) {
  }

  Panic();
  return False;
}
