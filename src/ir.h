#ifndef IR_H
#define IR_H

// Motivation for IR
// -----------------
//
// Having a 'proxy' layer between the frontend AST and the final code format provides flexibility.
// Blu supports compile time code execution so _something_ needs to be executed at compile time.
// Having an IR has several benefits.
// - Different language constructs from the AST can be mapped to the same IR instructions.
//   For example, different loop constructs (for, while) can be mapped to the same thing in IR.
// - The code can be made "more explicit". For example, Blu supports type coercion, but at some
//   point these implicit casts need to be made explicit. This can happen in the AST -> IR step.
// - IR is easier to interpret than the AST because it has simpler constructs.
// - IR is easier to translate to another target, like C or LLVM.
//
// For now, the idea is to convert AST -> IR.
// Interpret IR for compile time execution and then convert IR -> C 
// If interpreting IR is really slow in some cases, I see two alternatives atm
// 1. Convert to C, compile C, load dynamic lib and execute.
// 2. Use something in-process to compile and run the code, like MIR or create own dumb code generator.
//
// The first option is less work, because there will already be a C conversion.
// The second option is more work, because there will need to be written a new translation.
// But it has potential to be faster in total (should be faster to compile and no need to call an
// external C compiler etc.).
//
// Still, I would first like to write an IR interpreter.
//
// 1. IR interpreter
// 2. Partially evaluate comptime IR
// 3. Translate IR to C

#include "blu.h"

enum IrResult {
  IrResult_ok,
};

// The MSB of the `IrRef` encodes if it is an `InstructionIndex` or a `ValueIndex`.
// If the MSB is 1, then the `IrRef` is a `ValueIndex`.
typedef u32 IrRef;
typedef u32 InstructionIndex;
typedef u32 ChunkIndex;

typedef struct {
  ChunkIndex       chunk_index;
  InstructionIndex instruction_index;
} IrLocation;

enum IrOpcode {
  IR_comptime_func, // data references `IrComptimeFunc` in extra 
  IR_func,      // data references `IrFunc` in extra
  IR_arg,       // data contains `TypeIndex`
  IR_const,     // data contains `ValueIndex`
  IR_alloc,     // data contains `TypeIndex`
  IR_cond_br,   // data references `IrCondBr` in extra
  IR_block,     // data contains instruction count of block
  IR_loop,      // data contains instruction count of block
  IR_br,        // data contains `InstructionIndex`
  IR_repeat,    // data contains `InstructionIndex`
  IR_ret,       // data contains `IrRef`
  IR_load,      // data contains `IrRef`
  IR_store,     // data references `IrStore` in extra
  IR_call,      // data references `IrCall` in extra

  IR_declaration, // data references `IrDeclaration` in extra
  IR_lookup,
  
  IR_cast_int,  // data references `IrCastInt` in extra
  IR_cast_int_safe,
  IR_check_is_coercible,

  // Create a type
  IR_type, // 

  // Emit blocks do not return a value and can be seen as a marker to the compiler on what code to emit.
  // Emit blocks are only valid in a comptime function.
  IR_emit, // data contains how many instructions are in this block.

  // Eval blocks return a value and can only be exited with a `br`.
  IR_eval, // data contains how many instructions are in this block.
};

typedef struct {
  InstructionIndex declared_type; // optional
  InstructionIndex value;
} IrDeclaration;

typedef struct {
  TypeIndex return_type;
  u32 runtime_arg_count;
  u32 comptime_arg_count;
  u32 instruction_count;
} IrComptimeFunc;

typedef struct {
  TypeIndex return_type;
  u32 arg_count;
  u32 instruction_count;
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
  InstructionIndex then;
  InstructionIndex otherwise;
} IrCondBr;

typedef struct {
  IrRef dst;
  IrRef value;
} IrStore;

typedef struct {
  IrLocation func;
  u32 arg_count;
  IrRef args[];
} IrCall;

// ---

typedef struct {
  u32   opcode_count;
  Arena opcodes; // u8
  Arena data; // u32
  Arena extra; // variable
} IrChunk;

u8    opcode(IrChunk *chunk, InstructionIndex idx);
u32   instruction_data(IrChunk *chunk, InstructionIndex idx);
void *instruction_extra(IrChunk *chunk, InstructionIndex idx);

u32 generate_ir(Source *source);

#define ChunkList_min_size_log2   6
#define ChunkList_segment_count   24
#define SEGMENTLIST_NAME          ChunkList
#define SEGMENTLIST_TYPE          IrChunk
#define SEGMENTLIST_MIN_SIZE_LOG2 ChunkList_min_size_log2
#define SEGMENTLIST_SEGMENT_COUNT ChunkList_segment_count
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

typedef struct {
  Arena     *arena;
  ChunkList  list;
} IrChunkAllocator;

IrChunk *get_chunk(IrChunkAllocator *chunks, ChunkIndex idx);

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
  IrLocation           pc;
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

typedef struct {
  IrChunkAllocator *chunks;
  ValueStore       *values;
  TypeInterner     *types;
  CallStack         callstack;
  ValueStack        value_stack;
  ValueIndex        return_value;

  Arena *arena_value_stack;
  Arena *arena_callstack;
  Allocator allocator_inst_map;
} IrMachine;

void ir_machine_init(IrMachine *machine);
void ir_machine_deinit(IrMachine *machine);

u32 ir_run(IrMachine *machine);

#endif // IR_H
