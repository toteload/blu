#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "blu.h"
#include "ir.h"
#include "messages.h"

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
  ValueStore *values;
  CallStack2 call_stack;
} Interpreter2;

u32 interpreter_call(Interpreter2* in, IrChunk *chunk, ValueIndex *args, u32 arg_count, ValueIndex *out);

#endif // INTERPRETER_H
