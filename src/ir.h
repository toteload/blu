#ifndef IR_H
#define IR_H

#include <stdio.h>

#include "blu.h"
#include "types.h"

enum IrResult {
  IrResult_ok,
};

// -------------------------------------------------------------------------------------------------

// The MSB of the `IrRef` encodes if it is an `InstructionIndex` or a `ValueIndex`.
// If the MSB is 1, then the `IrRef` is a `InstructionIndex`.
// The reasoning behind this is that `InstructionIndex` is treated as an offset and is non-optional.
// `ValueIndex` is optional and a value of 0 means nil.
// `IrRef` is also optional, so making the optional `IrRef` value map to the optional `ValueIndex`
// value seems wise.
typedef u32 IrRef;

#define Bitmask_ir_ref_is_instruction_index (Cast(u32, 1) << 31)

always_inline b32 ref_is_value_index(IrRef ref) {
  return (ref & Bitmask_ir_ref_is_instruction_index) == 0;
}

always_inline b32 ref_is_instruction_index(IrRef ref) {
  return (ref & Bitmask_ir_ref_is_instruction_index) != 0;
}

always_inline b32 ir_ref_is_nil(IrRef ref) { return ref == 0; }

always_inline ValueIndex ref_to_value_index(IrRef ref) { return ref; }

always_inline InstructionIndex ref_to_instruction_index(IrRef ref) {
  return ref & ~Bitmask_ir_ref_is_instruction_index;
}

always_inline IrRef ir_ref_from_instruction_index(InstructionIndex idx) {
  return idx | Bitmask_ir_ref_is_instruction_index;
}

always_inline IrRef ir_ref_from_value_index(ValueIndex idx) { return idx; }

// -------------------------------------------------------------------------------------------------

enum IrOpcode {
  IR_func,    // data references `IrFunc` in extra
  IR_param,   // data contains `IrRef`
  IR_alloc,   // data contains `TypeIndex`
  IR_cond_br, // data references `IrCondBr` in extra
  IR_block,   // data contains instruction count of block
  IR_loop,    // data contains instruction count of block

  // br is allowed to br arbitrarily high.
  IR_br,     // data references `IrBr` in extra
  IR_ret,    // data contains `IrRef`
  IR_repeat, // data contains `InstructionIndex`
  IR_load,   // data contains `IrRef`
  IR_store,  // data references `IrStore` in extra
  IR_call,   // data references `IrCall` in extra

  IR_declaration,   // data references `IrDeclaration` in extra
  IR_lookup_typeof, // data contains `DeclarationIndex`
  IR_lookup_value,  // data contains `DeclarationIndex`

  IR_as,    // data references `IrAs` in extra
  IR_unify, // data references `IrUnify` in extra

  IR_type, // data references `IrType` in extra

  IR_return_type, // data contains `IrRef`
  IR_param_type,  // data references `IrParamType` in extra
};

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

typedef struct {
  u32 param_count;
  u32 instruction_count;
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

// -------------------------------------------------------------------------------------------------

typedef struct {
  u32 opcode_count;
  u8 *opcodes;
  u32 *data;
  AstIndex *ast_source;
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
#define AST_SOURCE_LIST_TYPE AstIndex
#define SEGMENTLIST_NAME AST_SOURCE_LIST_NAME
#define SEGMENTLIST_TYPE AST_SOURCE_LIST_TYPE
#define SEGMENTLIST_MIN_SIZE_LOG2 AST_SOURCE_LIST_MIN_SIZE_LOG2
#define SEGMENTLIST_SEGMENT_COUNT AST_SOURCE_LIST_SEGMENT_COUNT
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

typedef struct {
  Arena *scratch;
  OpcodeList kinds;
  AstSourceList ast_source;
  InstDataList data;
} IrBuilder;

InstructionIndex inst_alloc(IrBuilder *builder);
void inst_set_opcode(IrBuilder *builder, InstructionIndex idx, u8 opcode);
void inst_set_ast_source(IrBuilder *builder, InstructionIndex idx, AstIndex source);
void inst_set_data(IrBuilder *builder, InstructionIndex idx, u32 data);
void *inst_push_data_raw(IrBuilder *builder, InstructionIndex idx, u32 size, u32 align);
void *inst_get_extra(IrBuilder *builder, InstructionIndex idx);
u32 inst_offset(IrBuilder *builder, InstructionIndex start);
InstructionIndex inst_block_begin(IrBuilder *builder);
void inst_block_end(IrBuilder *builder, InstructionIndex block, IrRef val);
InstructionIndex inst_as(IrBuilder *builder, IrRef type_destination, IrRef val, AstIndex source);

void irbuilder_flatten(IrBuilder *builder, Arena *arena, IrChunk *chunk);

#define inst_push_data(builder, idx, type)                                                         \
  inst_push_data_raw(builder, idx, sizeof(type), Align_of(type))

// -------------------------------------------------------------------------------------------------

void ir_chunk_print(FILE *out, IrChunk *chunk, Source *source, TypeInterner *types, ValueStore *values);
void type_index_print(FILE *out, TypeInterner *types, TypeIndex idx);
void value_print(FILE *out, Source *source, TypeInterner *types, ValueStore *values, ValueIndex idx);

#endif // IR_H
