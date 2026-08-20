#ifndef INTERPRET_H
#define INTERPRET_H

#include "blu.h"
#include "ir.h"
#include "messages.h"
#include "compiler.h"

typedef enum {
  IOP_func, // data references `IFunc`
  IOP_param, // data contains `TypeIndex`
  IOP_alloc, // data contains `TypeIndex`
  IOP_load, // data references `ILoad`
  IOP_store, // data references `IStore`
  IOP_block, // data references `IBlock`
  IOP_loop, // data contains instruction count
  IOP_condbr, // data references `ICondbr`
  IOP_br, // data references `IBr`
  IOP_repeat, // data contains `InstructionIndex` for the loop
  IOP_ret, // data references `IRet`
  IOP_call, // data references `ICall`
  IOP_builtin_debug, 
} IOpcode;

typedef struct { u32 x; } IRef;

typedef struct {
  u32 instruction_count;
  TypeIndex return_type;
} IFunc;

typedef struct {
  TypeIndex type;
  IRef ptr;
} ILoad;

typedef struct {
  TypeIndex type;
  IRef value;
  IRef ptr;
} IStore;

typedef struct {
  u32 instruction_count;
  TypeIndex type;
} IBlock;

typedef struct {
  IRef cond;
  InstructionIndex then;
  InstructionIndex otherwise;
} ICondbr;

typedef struct {
  TypeIndex type;
  IRef value;
  InstructionIndex block;
} IBr;

typedef struct {
  TypeIndex type;
  IRef value;
} IRet;

typedef struct {
  TypeIndex func_type;
  IRef func_ptr;
  u32 arg_count;
  IRef args[];
} ICall;

typedef struct {
  InstructionIndex start;
  InstructionIndex end;
} ScopeSpan2;

typedef Stack(ScopeSpan2) ScopeStack2;

typedef struct {
  ValueIndex *ret;
  IrChunk *chunk;
  ValueIndex *inst_values;
  ScopeStack2 scope_stack;
  ArenaSnapshot snapshot;
  InstructionIndex pc;
} CallFrame2;

typedef Stack(CallFrame2) CallStack2;

typedef struct {
  Arena *scratch;
  MessageSink *msg_sink;
  Compiler *compiler;
  CallStack2 call_stack;
} Interpreter;

u32 interpreter_call(Interpreter* in, IrChunk *chunk, ValueIndex *args, u32 arg_count, ValueIndex *out);

#endif // INTERPRET_H
