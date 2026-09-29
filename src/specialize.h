#ifndef SPECIALIZE_H
#define SPECIALIZE_H

#include "blu.h"
#include "compiler.h"

// For simplicity the maximum depths are a fixed number. This will likely change.
#define MAX_SCOPE_DEPTH 128
#define MAX_BREAKS_AND_RETURNS 64

typedef enum {
  Scope_block,
  Scope_comptime,
} ScopeKind;

typedef struct {
  u8 scope_kind;

  InstructionIndex start;
  InstructionIndex end;
  InstructionIndex pc;

  InstructionIndex residual; // if scope_kind == Scope_block then this refers to the block in residual code
  InstructionIndex condbr; // if this scope wraps an if/else then this refers to a SIR_condbr

  struct {
    u32 len;
    InstructionIndex sources[MAX_BREAKS_AND_RETURNS];
  } breaks_and_returns;
} ScopeSpan;

ScopeSpan *find_scope(ScopeSpan *spans, u32 count, InstructionIndex start_of_block);
void scope_add_break_or_return(ScopeSpan *scope, InstructionIndex source);

typedef struct {
  SIrChunk *chunk;

  IRef *inst_map;
  TypeIndex *inst_types;

  u32 comptime_depth;
  Stack(ScopeSpan) scopes;
} Frame;

always_inline b32 frame_is_comptime(Frame *frame) { return frame->comptime_depth > 0; }

ScopeSpan *push_scope(Frame *frame, ScopeKind kind, InstructionIndex start, u32 count);
ScopeSpan pop_scope(Frame *frame);

typedef struct {
  b8 requested_resolution;

  Frame frame;

  Declaration *decl;
  ResidualFunction *function; // optional, only set if specializing a function
} SpecializerState;

void specializer_state_init_decl(SpecializerState *state, Arena *arena, Declaration *decl);
void specializer_state_init_function(SpecializerState *state, Arena *arena, ResidualFunction *function);

typedef struct {
  Arena               *perm;
  Arena               *scratch;

  MessageSink         *msg_sink;

  DeclarationInterner *declarations;
  TypeInterner        *types;
  ValueStore          *values;
  Common              *common;
} Specializer;

typedef enum {
  Step_ok,
  Step_error,
  Step_resolve_declaration_value,
  Step_resolve_function_body,
  Step_leave_scope,
} StepResult;

typedef enum {
  Run_ok,
  Run_error = Step_error,

  // The pc of the callframe will be on a lookup instruction with the DeclarationIndex
  // which needs to be resolved.
  Run_resolve_declaration_value = Step_resolve_declaration_value,
} RunResult;

u32 run_toplevel_block(Specializer *in, SpecializerState *state);

#endif // SPECIALIZE_H
