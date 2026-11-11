#ifndef SIR_H
#define SIR_H

#include "blu.h"
#include "types.h"
#include "ir.h"

// Specializer IR
// -------------------------------------------------------------------------------------------------

#define REF_NAME SRef
#define REF_FUNCTION_PREFIX sref
#include "ref.h"

#define SREF_NIL ((SRef){0})

// clang-format off
typedef enum {
  SIR_func,              // references SIrFunc
  SIR_param,             // contains no data
  SIR_alloc,             // contains SRef to a type
  SIR_load,
  SIR_store,             // references SIrStore
  SIR_block,             // references SIrBlock
  SIR_loop,              // contains instruction count of block
  SIR_condbr,            // references SIrCondbr
  SIR_br,                // references SIrBr
  SIR_repeat,            // contains InstructionOffset of loop block to repeat
  SIR_ret,               // contains SRef to value to return
  SIR_call,              // references SIrCall
  SIR_logical_and,       // references SIrLogicalAndOr
  SIR_logical_or,        // references SIrLogicalAndOr
  SIR_mul,               // references SIrBinary
  SIR_div,               // references SIrBinary
  SIR_mod,               // references SIrBinary
  SIR_sub,               // references SIrBinary
  SIR_add,               // references SIrBinary
  SIR_cmp_eq,            // references SIrBinary
  SIR_cmp_ne,            // references SIrBinary
  SIR_cmp_gt,            // references SIrBinary
  SIR_cmp_ge,            // references SIrBinary
  SIR_cmp_lt,            // references SIrBinary
  SIR_cmp_le,            // references SIrBinary
  SIR_bitshift_left,     // references SIrBinary
  SIR_bitshift_right,    // references SIrBinary
  SIR_bit_and,           // references SIrBinary
  SIR_bit_or,            // references SIrBinary
  SIR_bit_xor,           // references SIrBinary
  SIR_index,
  SIR_negate,            // contains SRef
  SIR_not,               // contains SRef
  SIR_builtin_debug,     // contains SRef
  SIR_builtin_len,       // contains SRef to an array or slice
  SIR_comptime_block,    // contains instruction count of block
  SIR_get_decl_value,    // contains DeclarationKey
  SIR_comptime_alloc,    // contains SRef to a type
  SIR_as,                // references SIrAs
  SIR_cast,              // references SIrCast
  SIR_unify,             // references SIrUnify
  SIR_type,              // references SIrType
  SIR_typeof,            // contains SRef
  SIR_base_type,         // contains SRef
  SIR_typeof_param,      // references SIrTypeofParam
  SIR_return_type,       // contains no data, evaluates to the return type of the function it is in
} SIrOpcode;
// clang-format on

// A logical and/or is wrapped by a block, referenced by `block`.
// `lhs` refers to the LHS value of the operator.
// The RHS of the operator is immediately after this instruction.
// The RHS is zero or more instructions terminated by a br to `block`.
typedef struct {
  InstructionOffset block;
  SRef lhs;
} SIrLogicalAndOr;

typedef struct {
  u32 instruction_count;
  SRef result_type;
} SIrBlock;

typedef struct {
  u32 instruction_count;
  u32 param_count;
  InstructionOffset body;
  SRef type_hint;
} SIrFunc;

typedef struct {
  SRef dst;
  SRef value;
} SIrStore;

typedef struct {
  SRef cond;
  InstructionOffset otherwise;
} SIrCondbr;

typedef struct {
  InstructionOffset block;
  SRef value;
} SIrBr;

typedef struct {
  SRef func;
  u32 arg_count;
  SRef args[];
} SIrCall;

// Unify must result in a valid type otherwise it is considered an error.
// Unification of the following two function types results in error:
// (i32, ?) bool + (i32, ?) ? = error | the second parameter type is unknown
typedef struct {
  SRef type_lhs;
  SRef type_rhs;
} SIrUnify;

typedef struct {
  SRef type_dst;
  SRef val;
} SIrAs;

typedef struct {
  SRef type_dst;
  SRef val;
} SIrCast;

typedef struct {
  u8 kind; // TypeKind
  u32 arg_count;
  SRef args[];
} SIrType;

typedef struct {
  SRef function_type;
  u32 param_index;
} SIrTypeofParam;

typedef struct {
  SRef lhs;
  SRef rhs;
} SIrBinary;

// Specializer IR Chunk
// -------------------------------------------------------------------------------------------------

typedef struct {
  u32 opcode_count;
  u8 *opcodes;
  u32 *data;
  AstAndSourceIndex *sources;
  void *extra;
} SIrChunk;

always_inline u8 sir_chunk_op(SIrChunk *chunk, InstructionOffset idx) {
  return chunk->opcodes[idx];
}

always_inline u32 sir_chunk_data(SIrChunk *chunk, InstructionOffset idx) {
  return chunk->data[idx];
}

always_inline void *sir_chunk_extra(SIrChunk *chunk, InstructionOffset idx) {
  return ptr_offset(chunk->extra, chunk->data[idx]);
}

always_inline u32 sir_chunk_block_count(SIrChunk *chunk, InstructionOffset idx) {
  if (chunk->opcodes[idx] == SIR_block) {
    return Cast(SIrBlock *, sir_chunk_extra(chunk, idx))->instruction_count;
  }

  return chunk->data[idx];
}

// Specializer IR Builder
// -------------------------------------------------------------------------------------------------

typedef struct {
  Arena *scratch;
  OpcodeList kinds;
  AstSourceList ast_source;
  InstDataList data;
} SIrBuilder;

InstructionOffset sir_builder_add(SIrBuilder *builder, u8 op, SourceKey source_idx, AstIndex ast_idx);
SRef sir_builder_add_as(SIrBuilder *builder, SRef type_destination, SRef val, SourceKey source_idx, AstIndex ast_idx);

void *sir_builder_push_data_raw(SIrBuilder *builder, InstructionOffset idx, u32 size, u32 align);

#define sir_builder_push_data(builder, idx, type)                                                  \
  sir_builder_push_data_raw(builder, idx, sizeof(type), Align_of(type))

void sir_builder_set_data(SIrBuilder *builder, InstructionOffset idx, u32 data);

u8 sir_builder_get_opcode(SIrBuilder *builder, InstructionOffset idx);

InstructionOffset sir_builder_add_block(SIrBuilder *builder, SRef result_type, SourceKey source_idx, AstIndex ast_idx);

void sir_builder_end_block(SIrBuilder *builder, InstructionOffset block);

InstructionOffset sir_builder_end_block_with(SIrBuilder *builder, InstructionOffset block, InstructionOffset target, SRef ref, SourceKey source_idx, AstIndex ast_idx);

u32 sir_builder_offset(SIrBuilder *builder, InstructionOffset idx);
void sir_builder_flatten(SIrBuilder *builder, Arena *arena, SIrChunk *chunk);

#endif // SIR_H
