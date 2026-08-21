#ifndef IR_H
#define IR_H

#include "blu.h"

typedef struct {
  SourceIndex source_idx;
  AstIndex    ast_idx;
} AstAndSourceIndex;

// Specializer IR
// -------------------------------------------------------------------------------------------------

#define REF_NAME SRef
#define REF_FUNCTION_PREFIX sref
#include "ref.h"

typedef enum {
  SIR_func,           // references SFunc
  SIR_param,          // contains SRef to a type
  SIR_alloc,          // contains SRef to a type
  SIR_load,
  SIR_store,          // references SStore
  SIR_block,          // contains instruction count of block
  SIR_loop,           // contains instruction count of block
  SIR_condbr,         // references SCondbr
  SIR_br,             // references SBr
  SIR_repeat,         // contains InstructionIndex of loop block to repeat
  SIR_ret,            // contains SRef to value to return
  SIR_call,           // references SCall
  SIR_builtin_debug, // contains SRef
  SIR_eval_block,     // contains instruction count of block
  SIR_lookup_decl_value,   // contains DeclarationIndex
  SIR_lookup_decl_type,  // contains DeclarationIndex
  SIR_comptime_alloc, // contains SRef to a type
  SIR_as,             // references SAs
  SIR_unify,          // references SUnify
  SIR_type,           // references SType
  SIR_typeof,         // contains SRef
  SIR_base_type,      // contains SRef
  SIR_return_type,    // contains SRef
  SIR_param_type,     // references SParamType
} SOpcode;

// A SOP_func instruction is followed by `param_count` SOP_param instructions.
typedef struct {
  u32 param_count;
  u32 instruction_count;
  SRef return_type;
} SFunc;

typedef struct {
  SRef dst;
  SRef value;
} SStore;

typedef struct {
  SRef cond;
  InstructionIndex then;
  InstructionIndex otherwise;
} SCondbr;

typedef struct {
  InstructionIndex block;
  SRef value;
} SBr;

typedef struct {
  SRef func;
  u32 arg_count;
  SRef args[];
} SCall;

// Unify must result in a valid type otherwise it is considered an error.
// Unification of the following two function types results in error:
// (i32, ?) bool + (i32, ?) ? = error | the second parameter type is unknown
typedef struct {
  SRef type_lhs;
  SRef type_rhs;
} SUnify;

typedef struct {
  SRef type_to;
  SRef val;
} SAs;

typedef struct {
  u8 kind; // TypeKind
  u32 arg_count;
  SRef args[];
} SType;

typedef struct {
  SRef function;
  u32 param_index;
} SParamType;

typedef struct {
  u32 opcode_count;
  u8 *opcodes;
  u32 *data;
  AstAndSourceIndex *sources;
  void *extra;
} SChunk;

always_inline u8 schunk_op(SChunk *chunk, InstructionIndex idx) {
  return chunk->opcodes[idx];
}

always_inline u32 schunk_data(SChunk *chunk, InstructionIndex idx) {
  return chunk->data[idx];
}

always_inline void *schunk_extra(SChunk *chunk, InstructionIndex idx) {
  return ptr_offset(chunk->extra, chunk->data[idx]);
}

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
  IIR_builtin_debug, // contains IRef
} IOpcode;

typedef struct {
  IRef ptr;
  IRef value;
} IStore;

typedef struct {
  IRef cond;
  InstructionIndex then;
  InstructionIndex otherwise;
} ICondbr;

typedef struct {
  IRef value;
  InstructionIndex block;
} IBr;

typedef struct {
  IRef func_ptr;
  u32 arg_count;
  IRef args[];
} ICall;

// -------------------------------------------------------------------------------------------------

typedef struct {
  u32 opcode_count;
  u8 *opcodes;
  u32 *data;
  TypeIndex *types;
  AstAndSourceIndex *sources;
  void *extra;
} IChunk;

always_inline u8 ichunk_op(IChunk *chunk, InstructionIndex idx) {
  return chunk->opcodes[idx];
}

always_inline TypeIndex ichunk_type(IChunk *chunk, InstructionIndex idx) {
  return chunk->types[idx];
}

always_inline u32 ichunk_data(IChunk *chunk, InstructionIndex idx) {
  return chunk->data[idx];
}

always_inline void *ichunk_extra(IChunk *chunk, InstructionIndex idx) {
  return ptr_offset(chunk->extra, chunk->data[idx]);
}

// -------------------------------------------------------------------------------------------------

#if 0
typedef struct { u32 x; } Ref;

// The MSB of the `IrRef` encodes if it is an `InstructionIndex` or a `ValueIndex`.
// If the MSB is 1, then the `IrRef` is a `InstructionIndex`.
// The reasoning behind this is that `InstructionIndex` is treated as an offset and is non-optional.
// `ValueIndex` is optional and a value of 0 means nil.
// `IrRef` is also optional, so making the optional `IrRef` value map to the optional `ValueIndex`
// value seems wise.
typedef Ref IrRef;

// Same as `IrRef` except the `InstructionIndex` variant of a `ResolvedRef` refers to the instruction
// index of the residual instructions not an instruction index in the comptime IR.
typedef Ref ResolvedRef;

#define BITMASK_REF_IS_INSTRUCTION_INDEX (Cast(u32, 1) << 31)

always_inline b32 ref_is_value_index(Ref r) { return (r.x & BITMASK_REF_IS_INSTRUCTION_INDEX) == 0; }
always_inline b32 ref_is_some_value_index(Ref r) { return r.x != 0 && ref_is_value_index(r); }
always_inline b32 ref_is_instruction_index(Ref r) { return (r.x & BITMASK_REF_IS_INSTRUCTION_INDEX) != 0; }
always_inline b32 ref_is_nil(Ref r) { return r.x == 0; }
always_inline ValueIndex ref_to_value_index(Ref r) { return r.x; }
always_inline ValueIndex ref_to_instruction_index(Ref r) { return r.x & ~BITMASK_REF_IS_INSTRUCTION_INDEX; }
always_inline u32 ref_from_instruction_index(InstructionIndex idx) { return idx | BITMASK_REF_IS_INSTRUCTION_INDEX; }
always_inline u32 ref_from_value_index(ValueIndex idx) { return idx; }
always_inline u32 ref_to_u32(Ref r) { return r.x; }

always_inline ResolvedRef resolved_ref_from_instruction_index(InstructionIndex idx) {
  return (ResolvedRef){ ref_from_instruction_index(idx) };
}

always_inline ResolvedRef resolved_ref_from_value_index(ValueIndex idx) {
  return (ResolvedRef){ ref_from_value_index(idx) };
}

always_inline IrRef ir_ref_from_instruction_index(InstructionIndex idx) {
  return (IrRef){ ref_from_instruction_index(idx) };
}

always_inline IrRef ir_ref_from_value_index(ValueIndex idx) {
  return (IrRef){ ref_from_value_index(idx) };
}

// -------------------------------------------------------------------------------------------------

typedef enum {
  IR_nop,

  IR_func,   // data references `IrFunc` in extra
  IR_param,  // data contains `IrRef`
  IR_alloc,  // data contains `IrRef`
  IR_condbr, // data references `IrCondBr` in extra
  IR_block,  // data contains instruction count of block
  IR_eval_block, // data contains instruction count of block
  IR_loop,   // data contains instruction count of block

  // br is allowed to br arbitrarily high.
  IR_br,     // data references `IrBr` in extra
  IR_ret,    // data contains `IrRef`
  IR_repeat, // data contains `InstructionIndex`
  IR_load,   // data contains `IrRef`
  IR_store,  // data references `IrStore` in extra
  IR_call,   // data references `IrCall` in extra

  IR_builtin_debug, // data contains `IrRef`

  IR_declaration,   // data references `IrDeclaration` in extra
  IR_lookup_typeof, // data contains `DeclarationIndex`
  IR_lookup_value,  // data contains `DeclarationIndex`

  IR_comptime_alloc, // data contains `IrRef`

  IR_as,    // data references `IrAs` in extra
  IR_unify, // data references `IrUnify` in extra

  IR_type, // data references `IrType` in extra
  IR_typeof, // data contains `IrRef`

  IR_base_type, // data contains `IrRef`
  IR_return_type, // data contains `IrRef`
  IR_param_type,  // data references `IrParamType` in extra
} IrOpcode;

// -------------------------------------------------------------------------------------------------

typedef struct {
  IrRef function;
  u32 param_index;
} IrParamType;

typedef struct {
  u8 kind; // TypeKind
  u32 arg_count;
  IrRef args[];
} IrType;

// You can put the value directly after the declaration instruction by convention.
// `data` can then hold an optional ref to the optional declared type.
// This way you don't need this IrDeclaration.
typedef struct {
  IrRef declared_type;
  IrRef value;
} IrDeclaration;

// Unify must result in a valid type otherwise it is considered an error.
// Unification of the following two function types results in error:
// (i32, ?) bool + (i32, ?) ? = error | the second parameter type is unknown
typedef struct {
  IrRef type_lhs;
  IrRef type_rhs;
} IrUnify;

typedef struct {
  IrRef type_to;
  IrRef val;
} IrAs;

// An IR_func instruction is followed by `param_count` IR_param instructions.
typedef struct {
  u32 param_count;
  u32 instruction_count;
  IrRef return_type;
} IrFunc;

typedef struct {
  InstructionIndex block;
  IrRef value;
} IrBr;

typedef struct {
  IrRef cond;
  InstructionIndex then;
  InstructionIndex otherwise;
} IrCondBr;

typedef struct {
  IrRef dst;
  IrRef value;
} IrStore;

typedef struct {
  IrRef func;
  u32 arg_count;
  IrRef args[];
} IrCall;
#endif

// -------------------------------------------------------------------------------------------------

typedef struct {
  u32 opcode_count;
  u8 *opcodes;
  u32 *data;
  AstAndSourceIndex *sources;
  void *extra;
} IrChunk;

u8 chunk_opcode(IrChunk *chunk, InstructionIndex idx);
u32 chunk_data(IrChunk *chunk, InstructionIndex idx);
void *chunk_extra(IrChunk *chunk, InstructionIndex idx);

// -------------------------------------------------------------------------------------------------

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

typedef struct {
  Arena *scratch;
  OpcodeList kinds;
  TypeList types;
  AstSourceList ast_source;
  InstDataList data;
} IIrBuilder;

typedef struct {
  Arena *scratch;
  OpcodeList kinds;
  AstSourceList ast_source;
  InstDataList data;
} IrBuilder;

InstructionIndex irbuilder_add(IrBuilder *builder, u8 op);
InstructionIndex irbuilder_add_sir_as(IrBuilder *builder, SRef type_destination, SRef val);
void irbuilder_end_sir_block_with(IrBuilder *builder, InstructionIndex block, InstructionIndex target, SRef ref);

//InstructionIndex inst_alloc(IrBuilder *builder);

void inst_set_opcode(IrBuilder *builder, InstructionIndex idx, u8 opcode);
void inst_set_source(IrBuilder *builder, InstructionIndex idx, SourceIndex source_idx, AstIndex ast_idx);
void inst_set_data(IrBuilder *builder, InstructionIndex idx, u32 data);
void *inst_push_data_raw(IrBuilder *builder, InstructionIndex idx, u32 size, u32 align);

u8 inst_get_opcode(IrBuilder *builder, InstructionIndex idx);
u32 inst_get_data(IrBuilder *builder, InstructionIndex idx);
void *inst_get_extra(IrBuilder *builder, InstructionIndex idx);
u32 inst_offset(IrBuilder *builder, InstructionIndex start);

//InstructionIndex inst_loop_begin(IrBuilder *builder);
//InstructionIndex inst_block_begin(IrBuilder *builder);
//InstructionIndex inst_eval_block_begin(IrBuilder *builder);
//void inst_block_end(IrBuilder *builder, InstructionIndex block);
//void inst_block_end_with_value(IrBuilder *builder, InstructionIndex block, IrRef ref);
//void inst_block_end_with_target(IrBuilder *builder, InstructionIndex block, InstructionIndex target);
//void inst_block_end_with_value_and_target(IrBuilder *builder, InstructionIndex block, InstructionIndex target, IrRef ref);
//void inst_block_end_repeat(IrBuilder *builder, InstructionIndex block, InstructionIndex target);
//
//InstructionIndex inst_as(IrBuilder *builder, IrRef type_destination, IrRef val, SourceIndex source_idx, AstIndex source);

void irbuilder_flatten(IrBuilder *builder, Arena *arena, IrChunk *chunk);

#define inst_push_data(builder, idx, type)                                                         \
  inst_push_data_raw(builder, idx, sizeof(type), Align_of(type))

#endif // IR_H
