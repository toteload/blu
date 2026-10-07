#ifndef SPECIALIZE_H
#define SPECIALIZE_H

#include "blu.h"
#include "compiler.h"

#define BREAK_RETURN_LIST_MIN_SIZE_LOG_2 4
#define BREAK_RETURN_LIST_SEGMENT_COUNT  12
#define BREAK_RETURN_LIST_NAME BreakReturnList
#define BREAK_RETURN_LIST_TYPE InstructionIndex
#define SEGMENTLIST_NAME BREAK_RETURN_LIST_NAME
#define SEGMENTLIST_TYPE BREAK_RETURN_LIST_TYPE
#define SEGMENTLIST_MIN_SIZE_LOG2 BREAK_RETURN_LIST_MIN_SIZE_LOG_2
#define SEGMENTLIST_SEGMENT_COUNT BREAK_RETURN_LIST_SEGMENT_COUNT
#define SEGMENTLIST_OUTPUT_TYPES
#define SEGMENTLIST_OUTPUT_DECLARATIONS
#include "segment_list.h"

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

  BreakReturnList breaks_and_returns;
} Scope;

#define SCOPE_STACK_MIN_SIZE_LOG_2 4
#define SCOPE_STACK_SEGMENT_COUNT  12
#define SCOPE_STACK_NAME ScopeStack
#define SCOPE_STACK_TYPE Scope
#define SEGMENTLIST_NAME SCOPE_STACK_NAME
#define SEGMENTLIST_TYPE SCOPE_STACK_TYPE
#define SEGMENTLIST_MIN_SIZE_LOG2 SCOPE_STACK_MIN_SIZE_LOG_2
#define SEGMENTLIST_SEGMENT_COUNT SCOPE_STACK_SEGMENT_COUNT
#define SEGMENTLIST_OUTPUT_TYPES
#define SEGMENTLIST_OUTPUT_DECLARATIONS
#include "segment_list.h"

typedef struct {
  DeclarationIndex decl;
  SIrChunk *chunk;

  IRef *inst_map;
  TypeIndex *inst_types;

  u32 comptime_depth;
  ScopeStack scopes;
} Frame;

void frame_init(Frame *frame, Arena *arena, Declaration *decl);

always_inline b32 frame_is_comptime(Frame *frame) { return frame->comptime_depth > 0; }

Scope *push_scope(Frame *frame, Arena *arena, ScopeKind kind, InstructionIndex start, u32 count);
Scope  pop_scope(Frame *frame);

typedef struct {
  IIrBuilder *builder;
  union {
    ValueIndex val;
    DeclarationIndex decl;
  } data;
  Frame frame;
} SpecializerState;

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
  Step_register_function,
  Step_leave_scope,
} StepResult;

typedef enum {
  Run_ok,
  Run_error = Step_error,

  // The pc of the callframe will be on a lookup instruction with the DeclarationIndex
  // which needs to be resolved.
  Run_resolve_declaration_value = Step_resolve_declaration_value,
  Run_register_function = Step_register_function,
} RunResultCode;

typedef struct {
  u32 code;

  // Run_ok                        - val ...
  // Run_error                     - none
  // Run_resolve_declaration_value - decl is the declaration that needs to be resolved
  // Run_register_function         - stub is the value of the function just encountered

  union {
    ValueIndex val;
    DeclarationIndex decl;
    ValueIndex stub;
  } data;
} RunResult;

RunResult run_toplevel_block(Specializer *sp, SpecializerState *state);

#endif // SPECIALIZE_H

