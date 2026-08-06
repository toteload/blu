#ifndef INTERPRET_H
#define INTERPRET_H

#include "blu.h"
#include "compiler.h"

// For simplicity the maximum depths are a fixed number. This will likely change.
#define MAX_SCOPE_DEPTH 64
#define MAX_CALL_DEPTH 128

#define MAX_BUILDERS 64 // Arbitrary number.
#define MAX_BREAKS_AND_RETURNS 16

typedef struct {
  u8 block_opcode; // block, eval_block, func
  InstructionIndex residual_block;

  InstructionIndex start;
  InstructionIndex end;
  InstructionIndex pc;

  struct {
    u32 len;
    ResolvedRef values[MAX_BREAKS_AND_RETURNS];
    InstructionIndex sources[MAX_BREAKS_AND_RETURNS];
  } breaks_and_returns;
} ScopeSpan;

void scope_add_break_or_return(ScopeSpan *scope, InstructionIndex source, ResolvedRef val);

typedef Stack(ScopeSpan) ScopeStack;

typedef struct {
  DeclarationIndex decl_idx;
  IrChunk *chunk;

  ResolvedRef *inst_map;

  ScopeStack scopes;

  ArenaSnapshot snapshot; // TODO remove this or actually do something with it
} CallFrame;

typedef Stack(CallFrame) CallStack;

typedef struct {
  b8 requested_resolution;
  CallStack call_stack;
} RunState;

void runstate_init(RunState *state, Arena *arena);
CallFrame *frame_push(RunState *state, Arena *arena, Declaration* decl);
void frame_pop(RunState *state, Arena *arena, ValueStore *values);
CallFrame *top_frame(RunState *state);
ScopeSpan *get_func_scope(CallFrame *frame);

typedef struct {
  Arena               *perm;
  Arena               *scratch;

  MessageSink         *msg_sink;

  DeclarationInterner *declarations;
  TypeInterner        *types;
  ValueStore          *values;
  Common              *common;

  Stack(IrBuilder)     builders;
} Interpreter;

typedef enum {
  Step_ok,
  Step_encountered_error,
  Step_resolve_declaration_type,
  Step_resolve_declaration_value,
} StepResult;

typedef enum {
  Run_reached_end,
  Run_encountered_error = Step_encountered_error,

  // The pc of the callframe will be on a lookup instruction with the DeclarationIndex
  // which needs to be resolved.
  Run_resolve_declaration_type = Step_resolve_declaration_type,
  Run_resolve_declaration_value = Step_resolve_declaration_value,
} RunResult;

u32 run_until(Interpreter *in, RunState *state, u32 inst_end);

#endif // INTERPRET_H
