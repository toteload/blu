#ifndef IR_H
#define IR_H

#include "blu.h"

typedef struct {
  SourceIndex source_idx;
  AstIndex    ast_idx;
} AstAndSourceIndex;

#define OPCODE_LIST_MIN_SIZE_LOG_2 8
#define OPCODE_LIST_SEGMENT_COUNT 24
#define OPCODE_LIST_NAME OpcodeList
#define OPCODE_LIST_TYPE u8
#define SEGMENTLIST_NAME OPCODE_LIST_NAME
#define SEGMENTLIST_TYPE OPCODE_LIST_TYPE
#define SEGMENTLIST_MIN_SIZE_LOG2 OPCODE_LIST_MIN_SIZE_LOG_2
#define SEGMENTLIST_SEGMENT_COUNT OPCODE_LIST_SEGMENT_COUNT
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

#define TYPE_LIST_MIN_SIZE_LOG_2 8
#define TYPE_LIST_SEGMENT_COUNT 24
#define TYPE_LIST_NAME TypeList
#define TYPE_LIST_TYPE TypeIndex
#define SEGMENTLIST_NAME TYPE_LIST_NAME
#define SEGMENTLIST_TYPE TYPE_LIST_TYPE
#define SEGMENTLIST_MIN_SIZE_LOG2 TYPE_LIST_MIN_SIZE_LOG_2
#define SEGMENTLIST_SEGMENT_COUNT TYPE_LIST_SEGMENT_COUNT
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

typedef union {
  u32 data;
  void *ptr;
} InstData;

#define INST_DATALIST_MIN_SIZE_LOG2 8
#define INST_DATALIST_SEGMENT_COUNT 24
#define INST_DATALIST_NAME InstDataList
#define INST_DATALIST_TYPE InstData
#define SEGMENTLIST_NAME INST_DATALIST_NAME
#define SEGMENTLIST_TYPE INST_DATALIST_TYPE
#define SEGMENTLIST_MIN_SIZE_LOG2 INST_DATALIST_MIN_SIZE_LOG2
#define SEGMENTLIST_SEGMENT_COUNT INST_DATALIST_SEGMENT_COUNT
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

#define AST_SOURCE_LIST_MIN_SIZE_LOG2 8
#define AST_SOURCE_LIST_SEGMENT_COUNT 24
#define AST_SOURCE_LIST_NAME AstSourceList
#define AST_SOURCE_LIST_TYPE AstAndSourceIndex
#define SEGMENTLIST_NAME AST_SOURCE_LIST_NAME
#define SEGMENTLIST_TYPE AST_SOURCE_LIST_TYPE
#define SEGMENTLIST_MIN_SIZE_LOG2 AST_SOURCE_LIST_MIN_SIZE_LOG2
#define SEGMENTLIST_SEGMENT_COUNT AST_SOURCE_LIST_SEGMENT_COUNT
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

// Specializer IR
// -------------------------------------------------------------------------------------------------

#define REF_NAME SRef
#define REF_FUNCTION_PREFIX sref
#include "ref.h"

typedef enum {
  SIR_func,           // references SIrFunc
  SIR_param,          // contains SRef to a type
  SIR_alloc,          // contains SRef to a type
  SIR_load,
  SIR_store,          // references SIrStore
  SIR_block,          // contains instruction count of block
  SIR_loop,           // contains instruction count of block
  SIR_condbr,         // references SIrCondbr
  SIR_br,             // references SIrBr
  SIR_repeat,         // contains InstructionIndex of loop block to repeat
  SIR_ret,            // contains SRef to value to return
  SIR_call,           // references SIrCall
  SIR_and,
  SIR_or,
  SIR_mul,
  SIR_div,
  SIR_mod,
  SIR_sub,
  SIR_add,
  SIR_cmp_eq,         // references SIrBinary
  SIR_cmp_ne,         // references SIrBinary
  SIR_cmp_gt,         // references SIrBinary
  SIR_cmp_ge,         // references SIrBinary
  SIR_cmp_lt,         // references SIrBinary
  SIR_cmp_le,         // references SIrBinary
  SIR_index,
  SIR_negate,         // contains SRef
  SIR_not,            // contains SRef
  SIR_builtin_debug,  // contains SRef
  SIR_eval_block,     // contains instruction count of block
  SIR_lookup_decl_value,   // contains DeclarationIndex
  SIR_lookup_decl_type,  // contains DeclarationIndex
  SIR_comptime_alloc, // contains SRef to a type
  SIR_as,             // references SIrAs
  SIR_unify,          // references SIrUnify
  SIR_type,           // references SIrType
  SIR_typeof,         // contains SRef
  SIR_base_type,      // contains SRef
  SIR_return_type,    // contains SRef
  SIR_param_type,     // references SIrParamType
} SIrOpcode;

// A SOP_func instruction is followed by `param_count` SOP_param instructions.
typedef struct {
  u32 param_count;
  u32 instruction_count;
  SRef return_type;
} SIrFunc;

typedef struct {
  SRef dst;
  SRef value;
} SIrStore;

typedef struct {
  SRef cond;
  InstructionIndex then;
  InstructionIndex otherwise;
} SIrCondbr;

typedef struct {
  InstructionIndex block;
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
  SRef type_to;
  SRef val;
} SIrAs;

typedef struct {
  u8 kind; // TypeKind
  u32 arg_count;
  SRef args[];
} SIrType;

typedef struct {
  SRef function;
  u32 param_index;
} SIrParamType;

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

always_inline u8 sir_chunk_op(SIrChunk *chunk, InstructionIndex idx) {
  return chunk->opcodes[idx];
}

always_inline u32 sir_chunk_data(SIrChunk *chunk, InstructionIndex idx) {
  return chunk->data[idx];
}

always_inline void *sir_chunk_extra(SIrChunk *chunk, InstructionIndex idx) {
  return ptr_offset(chunk->extra, chunk->data[idx]);
}

// Specializer IR Builder
// -------------------------------------------------------------------------------------------------

typedef struct {
  Arena *scratch;
  OpcodeList kinds;
  AstSourceList ast_source;
  InstDataList data;
} SIrBuilder;

InstructionIndex sir_builder_add(SIrBuilder *builder, u8 op);
InstructionIndex sir_builder_add_as(SIrBuilder *builder, SRef type_destination, SRef val);

void *sir_builder_push_data_raw(SIrBuilder *builder, InstructionIndex idx, u32 size, u32 align);

#define sir_builder_push_data(builder, idx, type)                                                  \
  sir_builder_push_data_raw(builder, idx, sizeof(type), Align_of(type))

void sir_builder_set_source(SIrBuilder *builder, InstructionIndex idx, SourceIndex source_idx, AstIndex ast_idx);
void sir_builder_set_data(SIrBuilder *builder, InstructionIndex idx, u32 data);

void sir_builder_end_block_with(SIrBuilder *builder, InstructionIndex block, InstructionIndex target, SRef ref);

u32 sir_builder_offset(SIrBuilder *builder, InstructionIndex idx);
void sir_builder_flatten(SIrBuilder *builder, Arena *arena, SIrChunk *chunk);

// Interpreter IR
// -------------------------------------------------------------------------------------------------

#define REF_NAME IRef
#define REF_FUNCTION_PREFIX iref
#include "ref.h"

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
  IIR_repeat, // contains `InstructionIndex` for the loop
  IIR_ret, // contains Iref
  IIR_call, // references `ICall`
  IIR_int_add,
  IIR_builtin_debug, // contains IRef
} IIrOpcode;

typedef struct {
  IRef ptr;
  IRef value;
} IIrStore;

typedef struct {
  IRef cond;
  InstructionIndex then;
  InstructionIndex otherwise;
} IIrCondbr;

typedef struct {
  IRef value;
  InstructionIndex block;
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
  TypeIndex *types;
  AstAndSourceIndex *sources;
  void *extra;
} IIrChunk;

always_inline u8 iir_chunk_op(IIrChunk *chunk, InstructionIndex idx) {
  return chunk->opcodes[idx];
}

always_inline TypeIndex iir_chunk_type(IIrChunk *chunk, InstructionIndex idx) {
  return chunk->types[idx];
}

always_inline u32 iir_chunk_data(IIrChunk *chunk, InstructionIndex idx) {
  return chunk->data[idx];
}

always_inline void *iir_chunk_extra(IIrChunk *chunk, InstructionIndex idx) {
  return ptr_offset(chunk->extra, chunk->data[idx]);
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

InstructionIndex iir_builder_add(IIrBuilder *builder, u8 op);
InstructionIndex iir_builder_add_as(IIrBuilder *builder, SRef type_destination, SRef val);
void iir_builder_end_block_with(IIrBuilder *builder, InstructionIndex block, InstructionIndex target, SRef ref);

void *iir_builder_push_data_raw(IIrBuilder *builder, InstructionIndex idx, u32 size, u32 align);

#define iir_builder_push_data(builder, idx, type)                                                  \
  iir_builder_push_data_raw(builder, idx, sizeof(type), Align_of(type))

void iir_builder_set_source(IIrBuilder *builder, InstructionIndex idx, SourceIndex source_idx, AstIndex ast_idx);
void iir_builder_set_data(IIrBuilder *builder, InstructionIndex idx, u32 data);
void iir_builder_set_type(IIrBuilder *builder, InstructionIndex idx, TypeIndex type);

u8 iir_builder_get_opcode(IIrBuilder *builder, InstructionIndex idx);
u32 iir_builder_get_data(IIrBuilder *builder, InstructionIndex idx);
TypeIndex iir_builder_get_type(IIrBuilder *builder, InstructionIndex idx);

u32 iir_builder_offset(IIrBuilder *builder, InstructionIndex idx);
void iir_builder_flatten(IIrBuilder *builder, Arena *arena, IIrChunk *chunk);

#endif // IR_H
