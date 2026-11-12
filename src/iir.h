#ifndef IIR_H
#define IIR_H

#include "blu.h"
#include "types.h"
#include "ir.h"

#define TYPE_LIST_MIN_SIZE_LOG_2 8
#define TYPE_LIST_SEGMENT_COUNT 24
#define TYPE_LIST_NAME TypeList
#define TYPE_LIST_TYPE TypeKey
#define SEGMENTLIST_NAME TYPE_LIST_NAME
#define SEGMENTLIST_TYPE TYPE_LIST_TYPE
#define SEGMENTLIST_MIN_SIZE_LOG2 TYPE_LIST_MIN_SIZE_LOG_2
#define SEGMENTLIST_SEGMENT_COUNT TYPE_LIST_SEGMENT_COUNT
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

// Interpreter IR
// -------------------------------------------------------------------------------------------------

#define REF_NAME IRef
#define REF_FUNCTION_PREFIX iref
#include "ref.h"

#define IREF_NIL ((IRef){0})

typedef enum {
  IIR_func, // contains instruction_count
  IIR_param, // data unused
  IIR_alloc, // data unused
  IIR_load, // contains IRef
  IIR_store, // references `IStore`
  IIR_block, // contains instruction_count
  IIR_loop, // contains instruction count
  IIR_condbr, // references `ICondbr`
  IIR_br, // contains IRef
  IIR_br_void,
  IIR_repeat, // contains `InstructionOffset` for the loop
  IIR_ret, // contains IRef
  IIR_ret_void,
  IIR_call, // references `ICall`

  IIR_int_extend, // contains IRef
  IIR_int_cast_check, // contains IRef
  IIR_int_add,
  IIR_int_sub,
  IIR_int_mul,
  IIR_int_div,
  IIR_int_mod,
  IIR_bit_not, // contains IRef
  IIR_bit_and,
  IIR_bit_or,
  IIR_bit_xor,
  IIR_int_cmp_eq,
  IIR_int_cmp_ne,
  IIR_int_cmp_gt,
  IIR_int_cmp_ge,
  IIR_int_cmp_lt,
  IIR_int_cmp_le,
  IIR_builtin_debug, // contains IRef
  IIR_builtin_len, // contains IRef to a slice
  IIR_index, // references IIrBinary, lhs is an array or slice
} IIrOpcode;

typedef struct {
  IRef ptr;
  IRef value;
} IIrStore;

typedef struct {
  IRef cond;
  InstructionOffset otherwise;
} IIrCondbr;

typedef struct {
  IRef value;
  InstructionOffset block;
} IIrBr;

typedef struct {
  IRef func_ptr;
  u32 arg_count;
  IRef args[];
} IIrCall;

typedef struct {
  IRef lhs;
  IRef rhs;
} IIrBinary;

// Interpreter IR Chunk
// -------------------------------------------------------------------------------------------------

typedef struct {
  u32 opcode_count;
  u8 *opcodes;
  u32 *data;
  void *extra;
  TypeKey *types;
  AstAndSourceIndex *sources;
} IIrChunk;

always_inline u8 iir_chunk_op(IIrChunk *chunk, InstructionOffset idx) {
  return chunk->opcodes[idx];
}

always_inline TypeKey iir_chunk_type(IIrChunk *chunk, InstructionOffset idx) {
  return chunk->types[idx];
}

always_inline u32 iir_chunk_data(IIrChunk *chunk, InstructionOffset idx) {
  return chunk->data[idx];
}

always_inline void *iir_chunk_extra(IIrChunk *chunk, InstructionOffset idx) {
  return ptr_offset(chunk->extra, chunk->data[idx]);
}

always_inline IRef iir_chunk_ref(IIrChunk *chunk, InstructionOffset idx) {
  return (IRef){chunk->data[idx]};
}

// Interpreter IR Builder
// -------------------------------------------------------------------------------------------------

typedef struct {
  Arena *scratch;
  OpcodeList kinds;
  TypeList types;
  AstSourceList ast_source;
  InstDataList data;
} IIrBuilder;

InstructionOffset iir_builder_add(IIrBuilder *builder, u8 op, AstAndSourceIndex source);

void *iir_builder_push_data_raw(IIrBuilder *builder, InstructionOffset idx, u32 size, u32 align);

#define iir_builder_push_data(builder, idx, type)                                                  \
  iir_builder_push_data_raw(builder, idx, sizeof(type), Align_of(type))

void iir_builder_set_data(IIrBuilder *builder, InstructionOffset idx, u32 data);
void iir_builder_set_type(IIrBuilder *builder, InstructionOffset idx, TypeKey type);

u8 iir_builder_get_opcode(IIrBuilder *builder, InstructionOffset idx);
u32 iir_builder_get_data(IIrBuilder *builder, InstructionOffset idx);
TypeKey iir_builder_get_type(IIrBuilder *builder, InstructionOffset idx);

u32 iir_builder_offset(IIrBuilder *builder, InstructionOffset idx);
void iir_builder_flatten(IIrBuilder *builder, Arena *arena, IIrChunk *chunk);

#endif // IIR_H
