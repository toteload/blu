#ifndef IR_H
#define IR_H

#include "blu.h"

// The MSB of the `IrRef` encodes if it is an `InstructionIndex` or a `ValueIndex`.
// If the MSB is 1, then the `IrRef` is a `ValueIndex`.
typedef u32 IrRef;
typedef u32 InstructionIndex;
typedef u32 ChunkIndex;

typedef struct {
  ChunkIndex       chunk_index;
  InstructionIndex instruction_index;
} IrLocation;

enum IrResult {
  IrResult_ok,
};

enum IrOpcode {
  IR_func,      // data references `IrFunc` in extra
  IR_arg,       // data contains `TypeIndex`
  IR_alloc,     // data contains `TypeIndex`
  IR_cond_br,   // data references `IrCondBr` in extra
  IR_block,     // data contains instruction count of block
  IR_loop,      // data contains instruction count of block
  IR_br,        // data contains `InstructionIndex`
  IR_repeat,    // data contains `InstructionIndex`
  IR_ret,       // data references `IrBr`
  IR_load,      //
  IR_store,     // data references `IrStore` in extra
  IR_cast_int,  // data references `IrCastInt` in extra
  IR_call,

  IR_typeid,

  // Emit blocks do not return a value and can be seen as a marker to the compiler on what code
  // to emit.
  IR_emit, // data contains how many instructions are in this block.

  // Eval blocks return a value and can only be exited with a `br`.
  IR_eval, // data contains how many instructions are in this block.
};

typedef struct {
  TypeIndex return_type;
  u32 instruction_count;
  u32 arg_count;
} IrFunc;

typedef struct {
  TypeIndex type;
  IrRef value;
} IrCastInt;

typedef struct {
  InstructionIndex block;
  IrRef value;
} IrBr;

typedef struct {
  IrRef cond;
  IrRef then;
  IrRef otherwise;
} IrCondBr;

typedef struct {
  IrRef dst;
  IrRef value;
} IrStore;

// ---

typedef struct {
  u32   opcode_count;
  u8   *opcodes;
  u32  *data;
  void *extra;
} IrChunk;

u8    opcode(IrChunk *chunk, IrInstructionIndex idx);
u32   instruction_data(IrChunk *chunk, IrInstructionIndex idx);
void *instruction_extra(IrChunk *chunk, IrInstructionIndex idx);

#define SEGMENTLIST_NAME          ChunkList
#define SEGMENTLIST_TYPE          IrChunk
#define SEGMENTLIST_MIN_SIZE_LOG2 6
#define SEGMENTLIST_SEGMENT_COUNT 24
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

typedef struct {
  Arena *arena;
  ChunkList chunks;
} IrChunkAllocator;

IrChunk *get_chunk(IrChunkAllocator *chunks, ChunkIndex idx);

ChunkIndex translate_ast_to_ir();

#define SEGMENTLIST_NAME          CallStackList
#define SEGMENTLIST_TYPE          IrLocation
#define SEGMENTLIST_MIN_SIZE_LOG2 5
#define SEGMENTLIST_SEGMENT_COUNT 24
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

#define HASHMAP_NAME       InstructionResultMap
#define HASHMAP_KEY_TYPE   IrLocation
#define HASHMAP_VALUE_TYPE ValueIndex
#define HASHMAP_OUTPUT_TYPES
#include "hashmap.h"

typedef struct {
  IrChunkAllocator     *chunks;
  ValueStore           *values;
  InstructionResultMap  inst_map;
  CallStackList         callstack;
} IrMachine;

u32 ir_call_safe(IrMachine *machine, IrLocation function, u32 arg_count, ValueIndex *args, ValueIndex *result);

#endif // IR_H
