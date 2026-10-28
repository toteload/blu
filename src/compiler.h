#ifndef COMPILER_H
#define COMPILER_H

#include "blu.h"
#include "messages.h"
#include "string_interner.h"
#include "value.h"
#include "ir.h"
#include "cli_options.h"
#include "declaration_interner.h"

#define SOURCELIST_MIN_SIZE_LOG2  4
#define SOURCELIST_SEGMENT_COUNT  20
#define SEGMENTLIST_NAME          SourceList
#define SEGMENTLIST_TYPE          Source
#define SEGMENTLIST_MIN_SIZE_LOG2 SOURCELIST_MIN_SIZE_LOG2
#define SEGMENTLIST_SEGMENT_COUNT SOURCELIST_SEGMENT_COUNT
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

#define DECLIDXLIST_MIN_SIZE_LOG2 4
#define DECLIDXLIST_SEGMENT_COUNT 20
#define SEGMENTLIST_NAME          DeclIdxList
#define SEGMENTLIST_TYPE          DeclarationKey
#define SEGMENTLIST_MIN_SIZE_LOG2 DECLIDXLIST_MIN_SIZE_LOG2
#define SEGMENTLIST_SEGMENT_COUNT DECLIDXLIST_SEGMENT_COUNT
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

typedef enum {
  RESOLVE_STATUS_UNRESOLVED,
  RESOLVE_STATUS_RESOLVING,
  RESOLVE_STATUS_FULLY_RESOLVED,
  RESOLVE_STATUS_ERROR,
} ResolveStatus;

typedef struct {
  u8 resolve_status;
  DeclarationKey decl;
  InstructionOffset inst;
  TypeKey result_type;
  ValueKey val;
} Stub;

#define STUB_LIST_NAME StubList
#define STUB_LIST_TYPE Stub
#define STUB_LIST_MIN_SIZE_LOG2 5
#define STUB_LIST_SEGMENT_COUNT 20
#define SEGMENTLIST_NAME STUB_LIST_NAME
#define SEGMENTLIST_TYPE STUB_LIST_TYPE
#define SEGMENTLIST_MIN_SIZE_LOG2 STUB_LIST_MIN_SIZE_LOG2
#define SEGMENTLIST_SEGMENT_COUNT STUB_LIST_SEGMENT_COUNT
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

typedef struct {
  IIrChunk chunk;
} ResidualFunction;

#define RESIDUAL_FUNCTION_LIST_MIN_SIZE_LOG2 5
#define RESIDUAL_FUNCTION_LIST_SEGMENT_COUNT 20
#define RESIDUAL_FUNCTION_LIST_NAME ResidualFunctionList
#define RESIDUAL_FUNCTION_LIST_TYPE ResidualFunction
#define SEGMENTLIST_NAME RESIDUAL_FUNCTION_LIST_NAME
#define SEGMENTLIST_TYPE RESIDUAL_FUNCTION_LIST_TYPE
#define SEGMENTLIST_MIN_SIZE_LOG2 RESIDUAL_FUNCTION_LIST_MIN_SIZE_LOG2
#define SEGMENTLIST_SEGMENT_COUNT RESIDUAL_FUNCTION_LIST_SEGMENT_COUNT
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

typedef struct Compiler {
  Arena arena;
  Arena scratch;

  CLIOptions *options;

  SourceList sources;

  MessageList msg_list;
  MessageSink msg_sink;

  Common         common;
  ValueStore     values;
  StringInterner strings;
  TypeInterner   types;

  DeclarationInterner decls;
  DeclIdxList         user_decls;

  ResidualFunctionList functions;
  StubList stubs;
} Compiler;

void compiler_init(Compiler *compiler, CLIOptions *options);
void compiler_deinit(Compiler *compiler);

void compiler_add_sourcefile(Compiler *compiler, String filename);
Source *compiler_get_source(Compiler *compiler, SourceKey source_idx);

ResidualFunctionKey compiler_add_function(Compiler *compiler, ResidualFunction function);
ResidualFunction *compiler_get_function(Compiler *compiler, ResidualFunctionKey key);

StubKey compiler_add_stub(Compiler *compiler, DeclarationKey decl, InstructionOffset inst, TypeKey type);
Stub *compiler_get_stub(Compiler *compiler, StubKey key);

b32 lookup_identifier(DeclarationInterner *decls_keys, DeclarationKey *mods, u32 mod_count, StringKey name, DeclarationKey *out);

void compiler_print_all_messages(Compiler *compiler);

b32 compile(Compiler *compiler);

b32 run_main(Compiler *compiler);

#endif // COMPILER_H
