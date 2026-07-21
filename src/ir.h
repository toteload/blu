#ifndef IR_H
#define IR_H

#include <stdio.h>

#include "blu.h"
#include "types.h"

enum IrResult {
  IrResult_ok,
};

// The MSB of the `IrRef` encodes if it is an `InstructionIndex` or a `ValueIndex`.
// If the MSB is 1, then the `IrRef` is a `InstructionIndex`.
// The reasoning behind this is that `InstructionIndex` is treated as an offset and is non-optional.
// `ValueIndex` is optional and a value of 0 means nil.
// `IrRef` is also optional, so making the optional `IrRef` value map to the optional `ValueIndex` value
// seems wise.
typedef u32 IrRef;

#define Bitmask_ir_ref_is_instruction_index (Cast(u32, 1) << 31)

always_inline b32 ref_is_value_index(IrRef ref) {
  return (ref & Bitmask_ir_ref_is_instruction_index) == 0;
}

always_inline b32 ir_ref_is_nil(IrRef ref) {
  return ref == 0;
}

always_inline ValueIndex ref_to_value_index(IrRef ref) {
  return ref;
}

always_inline InstructionIndex ref_to_instruction_index(IrRef ref) {
  return ref & ~Bitmask_ir_ref_is_instruction_index;
}

always_inline IrRef ir_ref_from_instruction_index(InstructionIndex idx) {
  return idx | Bitmask_ir_ref_is_instruction_index;
}

always_inline IrRef ir_ref_from_value_index(ValueIndex idx) {
  return idx;
}

enum IrOpcode {
  IR_func,      // data references `IrFunc` in extra
  IR_param,     // data contains `IrRef`
  IR_alloc,     // data contains `TypeIndex`
  IR_cond_br,   // data references `IrCondBr` in extra
  IR_block,     // data contains instruction count of block
  IR_loop,      // data contains instruction count of block
  IR_br,        // data references `IrBr` in extra
  IR_ret,       // data contains `IrRef`
  IR_repeat,    // data contains `InstructionIndex`
  IR_load,      // data contains `IrRef`
  IR_store,     // data references `IrStore` in extra
  IR_call,      // data references `IrCall` in extra

  IR_declaration, // data references `IrDeclaration` in extra
  IR_lookup, // data contains `DeclarationIndex`
  
  IR_cast_int,  // data references `IrCastInt` in extra
  IR_cast_int_safe,

  IR_as, // data references `IrAs` in extra
  IR_unify, // data references `IrUnify` in extra

  // Create a type
  IR_type, // data references `IrType` in extra
  IR_typeof,
  IR_typeinfo,

  IR_function_return_type, // data contains `IrRef`
};

typedef struct {
  u8 kind; // TypeKind
  u32 arg_count;
  IrRef args[];
} IrType;

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
  IrRef type_from;
} IrAs;

typedef struct {
  IrRef return_type;
  u32 arg_offset;
  u32 arg_count;
  u32 instruction_count;
} IrFunc;

typedef struct {
  IrRef type;
  IrRef value;
} IrCastInt;

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

// ---

typedef struct {
  u32   opcode_count;
  u8   *opcodes;
  u32  *data;
  void *extra;
} IrChunk;

u8    opcode(IrChunk *chunk, InstructionIndex idx);
u32   instruction_data(IrChunk *chunk, InstructionIndex idx);
void *instruction_extra(IrChunk *chunk, InstructionIndex idx);

u32 generate_ir(Source *source);

void ir_chunk_print(FILE *out, IrChunk *chunk, TypeInterner *types, ValueStore *values);

// ---

enum ValueStackElementKind {
  ValueStackElement_marker_frame,
  ValueStackElement_marker_block,
  ValueStackElement_value,
};

typedef struct {
  u8 kind;
  InstructionIndex idx;
} ValueStackElement;

#define ValueStack_min_size_log2  5
#define ValueStack_segment_count  24
#define SEGMENTLIST_NAME          ValueStack
#define SEGMENTLIST_TYPE          ValueStackElement
#define SEGMENTLIST_MIN_SIZE_LOG2 ValueStack_min_size_log2
#define SEGMENTLIST_SEGMENT_COUNT ValueStack_segment_count
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

#define HASHMAP_NAME       InstructionResultMap
#define HASHMAP_KEY_TYPE   InstructionIndex
#define HASHMAP_VALUE_TYPE ValueIndex
#define HASHMAP_OUTPUT_TYPES
#include "hashmap.h"

typedef struct {
  InstructionResultMap inst_map;
} CallFrame;

#define CallStack_min_size_log2   5
#define CallStack_segment_count   24
#define SEGMENTLIST_NAME          CallStack
#define SEGMENTLIST_TYPE          CallFrame
#define SEGMENTLIST_MIN_SIZE_LOG2 CallStack_min_size_log2
#define SEGMENTLIST_SEGMENT_COUNT CallStack_segment_count
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

#endif // IR_H
