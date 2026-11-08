#ifndef SPECIALIZE_H
#define SPECIALIZE_H

#include "blu.h"
#include "compiler.h"

#define BREAK_RETURN_LIST_MIN_SIZE_LOG_2 4
#define BREAK_RETURN_LIST_SEGMENT_COUNT  12
#define BREAK_RETURN_LIST_NAME BreakReturnList
#define BREAK_RETURN_LIST_TYPE InstructionOffset
#define SEGMENTLIST_NAME BREAK_RETURN_LIST_NAME
#define SEGMENTLIST_TYPE BREAK_RETURN_LIST_TYPE
#define SEGMENTLIST_MIN_SIZE_LOG2 BREAK_RETURN_LIST_MIN_SIZE_LOG_2
#define SEGMENTLIST_SEGMENT_COUNT BREAK_RETURN_LIST_SEGMENT_COUNT
#define SEGMENTLIST_OUTPUT_TYPES
#define SEGMENTLIST_OUTPUT_DECLARATIONS
#include "segment_list.h"

typedef enum {
  Scope_comptime,
  Scope_signature_of_function,
  Scope_function,
  Scope_block,
} ScopeKind;

typedef struct {
  u8 scope_kind;

  InstructionOffset start;
  InstructionOffset end;
  InstructionOffset pc;

  InstructionOffset residual; // if scope_kind == Scope_block then this refers to the block in residual code
  InstructionOffset condbr; // if this scope wraps an if/else then this refers to a SIR_condbr
  InstructionOffset logical_and; // if this scope wraps a runtime `and` then this refers to the SIR_logical_and
  InstructionOffset residual_otherwise;

  TypeKey result_type; // if scope_kind == Scope_block, the type every br to this block is coerced to, or nil to unify the brs

  BreakReturnList breaks_and_returns;

  union {
    struct {
      StubKey stub;
    } signature_of_function;

    struct {
      TypeKey function_type;
      TypeKey return_type;
    } function;
  } data;
} Scope;

b8 scope_kind_is_comptime(ScopeKind kind);
void scope_add_break_or_return(Scope *scope, Arena *arena, InstructionOffset source);

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
  DeclarationKey decl;
  SIrChunk *chunk;

  IRef *inst_map;
  TypeKey *inst_types;

  u32 comptime_depth;
  ScopeStack scopes;
} Frame;

void   frame_init(Frame *frame, Arena *arena, Declaration *decl);
b32    frame_is_comptime(Frame *frame);
Scope *frame_push_scope(Frame *frame, Arena *arena, ScopeKind kind, InstructionOffset start, u32 count);
Scope  frame_pop_scope(Frame *frame);
Scope *frame_get_func_scope(Frame *f);

typedef struct {
  Frame frame;
  struct {
    IIrBuilder builder;
  } function;
} Specializer;

typedef struct {
  Arena               *perm;
  Arena               *scratch;

  MessageSink         *msg_sink;

  DeclarationInterner *declarations;
  TypeInterner        *types;
  ValueInterner       *values;
  Common              *common;
  Compiler            *compiler;
} SpecializerContext;

typedef enum {
  Step_next_step,
  Step_finished,
  Step_error,
  Step_resolve_declaration_value,
  Step_register_function,
} StepResultKind;

typedef struct {
  StepResultKind kind;
  union {
    struct {
      ValueKey val;
    } finished;
    struct {
      DeclarationKey decl;
    } resolve_decl;
    struct {
      StubKey stub;
    } register_function;
  } data;
} StepResult;

void specializer_init_function(SpecializerContext *sp, Specializer *state, StubKey stub_key);

StepResult run_toplevel_block(SpecializerContext *sp, Specializer *state);
void specializer_resolve_decl(Specializer *state, ValueKey decl_val);

#endif // SPECIALIZE_H

