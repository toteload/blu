#ifndef INTERPRET_H
#define INTERPRET_H

#include "blu.h"
#include "ir.h"
#include "messages.h"
#include "compiler.h"

typedef enum {
  IOP_func,
  IOP_param,
  IOP_alloc,
  IOP_load,
  IOP_store,
  IOP_block,
  IOP_loop,
  IOP_condbr,
  IOP_br,
  IOP_repeat,
  IOP_ret,
  IOP_call,
  IOP_builtin_debug,
} IOpcode;

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
