#ifndef INTERPRET_H
#define INTERPRET_H

#include "blu.h"
#include "compiler.h"

// For simplicity the maximum depths are a fixed number. This will likely change.
#define MAX_SCOPE_DEPTH 64
#define MAX_CALL_DEPTH  128

typedef struct {
  InstructionIndex start;
  InstructionIndex end;
} ScopeSpan;

typedef Stack(ScopeSpan) ScopeStack;

typedef struct {
  // This struct probably will also have to carry arguments for when function calls get added.
  // And return value (maybe?)

  b32 ok;
  InstructionIndex pc;
  IrChunk *chunk;
  ValueIndex *inst_map;
  ScopeStack scopes;
  ArenaSnapshot snapshot;
} CallFrame;

typedef Stack(CallFrame) CallStack;

typedef struct {
  Arena *perm;
  Arena *scratch;

  Common *common;
  MessageSink *msg_sink;
  DeclarationInterner *decls;
  ValueStore *values;
  TypeInterner *types;
} InterpretContext;

typedef struct {
  IrBuilder builder;
  CallStack call_stack;

  Arena *scratch;
  MessageSink *msg_sink;
  DeclarationInterner *declarations;
  TypeInterner *types;
  ValueStore *values;
  Common *common;
} Interpreter;

void frame_push(InterpretContext *ctx, CallStack *stack, IrChunk *chunk);
void frame_pop(InterpretContext *ctx, CallStack *stack);

void step(Interpreter *in, CallFrame *f);

typedef enum {
  Run_ok,
  Run_resolve_declaration,
} RunResult;

u32 run_until(..., CallFrame *f, u32 idx);

typedef struct {
} Resolver;

b32 resolve_declarations();

#endif // INTERPRET_H
