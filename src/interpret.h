#ifndef INTERPRET_H
#define INTERPRET_H

#include "blu.h"
#include "ir.h"
#include "messages.h"
#include "compiler.h"

typedef struct {
  InstructionIndex start;
  InstructionIndex end;
  ArenaSnapshot snapshot;
} ScopeSpan2;

typedef Stack(ScopeSpan2) ScopeStack2;

typedef struct {
  void *ret;
  IChunk *chunk;
  void **inst_values;
  ScopeStack2 scope_stack;
  InstructionIndex pc;
} CallFrame2;

typedef Stack(CallFrame2) CallStack2;

typedef struct {
  Arena *scratch;
  MessageSink *msg_sink;
  Compiler *compiler;
  CallStack2 call_stack;
} Interpreter;

u32 interpreter_call(Interpreter* in, IChunk *chunk, ValueIndex *args, u32 arg_count, void *out);

#endif // INTERPRET_H
