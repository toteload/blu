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
  Scope_block,
  Scope_comptime,
  Scope_function,
  Scope_signature,
} ScopeKind;

typedef struct {
  u8 scope_kind;

  InstructionOffset start;
  InstructionOffset end;
  InstructionOffset pc;

  InstructionOffset residual; // if scope_kind == Scope_block then this refers to the block in residual code
  InstructionOffset condbr; // if this scope wraps an if/else then this refers to a SIR_condbr

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
  DeclarationKey decl;
  SIrChunk *chunk;

  IRef *inst_map;
  TypeKey *inst_types;

  u32 comptime_depth;
  ScopeStack scopes;
} Frame;

void frame_init(Frame *frame, Arena *arena, Declaration *decl);

always_inline b32 frame_is_comptime(Frame *frame) { return frame->comptime_depth > 0; }

Scope *push_scope(Frame *frame, Arena *arena, ScopeKind kind, InstructionOffset start, u32 count);
Scope  pop_scope(Frame *frame);

typedef struct {
  Frame frame;
  struct {
    IIrBuilder builder;
    TypeKey    return_type;
  } function;
  ValueKey val; // TODO what is this used for again?
} Specializer;

typedef struct {
  Arena               *perm;
  Arena               *scratch;

  MessageSink         *msg_sink;

  DeclarationInterner *declarations;
  TypeInterner        *types;
  ValueStore          *values;
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
  u8 kind;
  union {
    struct {
      ValueKey val;
    } finished;
    struct {
      DeclarationKey decl;
    } resolve_decl;
    struct {
      DeclarationKey decl;
      InstructionOffset inst;
      TypeKey type_signature;
    } register_function;
  } data;
} StepResult;

void specializer_init_function(SpecializerContext *sp, Specializer *state, StubKey stub_key);

StepResult run_toplevel_block(SpecializerContext *sp, Specializer *state);
void specializer_resolve_decl(Specializer *state, ValueKey decl_val);

#endif // SPECIALIZE_H

