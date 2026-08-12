#ifndef INTERPRET_H
#define INTERPRET_H

#include "blu.h"
#include "compiler.h"

// For simplicity the maximum depths are a fixed number. This will likely change.
#define MAX_SCOPE_DEPTH 64
#define MAX_CALL_DEPTH 128

#define MAX_BUILDERS 64 // Arbitrary number.
#define MAX_BREAKS_AND_RETURNS 16

typedef enum {
  Scope_chunk,
  Scope_block,
  Scope_eval_block,
  Scope_func,
} ScopeKind;

typedef enum {
  Scope_condbr_has_evaluated_branches = 1 << 0,
} ScopeFlags;

typedef struct {
  u8 scope_kind;
  u8 flags;

  InstructionIndex start;
  InstructionIndex end;
  InstructionIndex pc;

  InstructionIndex residual; // if scope_kind == Scope_block then this is the block in residual code

  struct {
    u32 len;
    InstructionIndex sources[MAX_BREAKS_AND_RETURNS];
  } breaks_and_returns;
} ScopeSpan;

typedef Stack(ScopeSpan) ScopeStack;

ScopeSpan *find_scope(ScopeSpan *spans, u32 count, InstructionIndex start_of_block);
void scope_add_break_or_return(ScopeSpan *scope, InstructionIndex source);

typedef struct {
  DeclarationIndex decl_idx;
  IrChunk *chunk;

  ResolvedRef *inst_map;
  TypeIndex *inst_types;

  ScopeStack scopes;

  ArenaSnapshot snapshot; // TODO remove this or actually do something with it
} CallFrame;

ScopeSpan *push_scope(CallFrame *frame);

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
  Step_leave_scope,
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

u32 run_block(Interpreter *in, RunState *state);

#endif // INTERPRET_H
