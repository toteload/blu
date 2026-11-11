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

typedef struct { u32 x; } CRef;

#define CREF_INSTRUCTION_BIT (Cast(u32, 1) << 31)

always_inline b32 cref_is_instruction(CRef r) {
  return (r.x & CREF_INSTRUCTION_BIT) != 0;
}

always_inline b32 cref_is_constant(CRef r) {
  return (r.x & CREF_INSTRUCTION_BIT) == 0;
}

always_inline InstructionOffset cref_to_instruction(CRef r) {
  ASSERT_SAFE(cref_is_instruction(r));
  return r.x & ~CREF_INSTRUCTION_BIT;
}

always_inline u32 cref_to_constant(CRef r) {
  ASSERT_SAFE(cref_is_constant(r));
  return r.x;
}

always_inline CRef cref_from_instruction(InstructionOffset idx) {
  ASSERT_SAFE((idx & CREF_INSTRUCTION_BIT) == 0);
  return (CRef){idx | CREF_INSTRUCTION_BIT};
}

always_inline CRef cref_from_constant(u32 idx) {
  ASSERT_SAFE((idx & CREF_INSTRUCTION_BIT) == 0);
  return (CRef){idx};
}

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

  // int_cast is for widening integer casts that cannot fail.
  IIR_int_cast, // contains IRef
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

typedef struct {
  CRef ptr;
  CRef value;
} IIrChunkStore;

typedef struct {
  CRef cond;
  InstructionOffset otherwise;
} IIrChunkCondbr;

typedef struct {
  CRef value;
  InstructionOffset block;
} IIrChunkBr;

typedef struct {
  CRef func_ptr;
  u32 arg_count;
  CRef args[];
} IIrChunkCall;

typedef struct {
  CRef lhs;
  CRef rhs;
} IIrChunkBinary;

_Static_assert(sizeof(CRef) == sizeof(IRef), "CRef and IRef must have the same size.");
_Static_assert(sizeof(IIrChunkStore) == sizeof(IIrStore), "IIrChunkStore and IIrStore must have the same size.");
_Static_assert(sizeof(IIrChunkCondbr) == sizeof(IIrCondbr), "IIrChunkCondbr and IIrCondbr must have the same size.");
_Static_assert(sizeof(IIrChunkBr) == sizeof(IIrBr), "IIrChunkBr and IIrBr must have the same size.");
_Static_assert(sizeof(IIrChunkCall) == sizeof(IIrCall), "IIrChunkCall and IIrCall must have the same size.");
_Static_assert(sizeof(IIrChunkBinary) == sizeof(IIrBinary), "IIrChunkBinary and IIrBinary must have the same size.");

// Interpreter IR Chunk
// -------------------------------------------------------------------------------------------------

typedef struct {
  u32 size;
  u32 align;
} IIrOpSizeInfo;

typedef struct {
  usize len;
  void *data;
} IIrSlice;

typedef struct {
  TypeKey type;
  void *data;
} IIrConstant;

typedef struct {
  u32 opcode_count;
  u8 *opcodes;
  u32 *data;
  void *extra;
  TypeKey *types;
  IIrOpSizeInfo *size_infos;
  AstAndSourceIndex *sources;
  u32 constant_count;
  IIrConstant *constants;
} IIrChunk;

always_inline u8 iir_chunk_op(IIrChunk *chunk, InstructionOffset idx) {
  return chunk->opcodes[idx];
}

always_inline TypeKey iir_chunk_type(IIrChunk *chunk, InstructionOffset idx) {
  return chunk->types[idx];
}

always_inline IIrOpSizeInfo iir_chunk_size_info(IIrChunk *chunk, InstructionOffset idx) {
  return chunk->size_infos[idx];
}

always_inline u32 iir_chunk_data(IIrChunk *chunk, InstructionOffset idx) {
  return chunk->data[idx];
}

always_inline void *iir_chunk_extra(IIrChunk *chunk, InstructionOffset idx) {
  return ptr_offset(chunk->extra, chunk->data[idx]);
}

always_inline CRef iir_chunk_ref(IIrChunk *chunk, InstructionOffset idx) {
  return (CRef){chunk->data[idx]};
}

always_inline IIrConstant *iir_chunk_constant(IIrChunk *chunk, CRef r) {
  return &chunk->constants[cref_to_constant(r)];
}

typedef u32 (*IIrConstantFn)(void *context, ValueKey value);

void iir_chunk_convert_refs(IIrChunk *chunk, IIrConstantFn fn, void *context);

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
void iir_builder_flatten(IIrBuilder *builder, Arena *arena, TypeInterner *types, IIrChunk *chunk);

#endif // IIR_H
