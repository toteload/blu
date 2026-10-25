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

typedef struct {
  u8 resolve_status;
  DeclarationKey decl;
  InstructionOffset inst; // The SIR_func instruction in the chunk of `decl`
  TypeKey type;           // The declared type unified with the types written on the function value
  IIrChunk chunk;         // Only valid once fully resolved
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
} Compiler;

void compiler_init(Compiler *compiler, CLIOptions *options);
void compiler_deinit(Compiler *compiler);

void compiler_add_sourcefile(Compiler *compiler, String filename);
Source *compiler_get_source(Compiler *compiler, SourceKey source_idx);

ResidualFunctionKey compiler_add_function(Compiler *compiler, ResidualFunction function);
ResidualFunction *compiler_get_function(Compiler *compiler, ResidualFunctionKey key);

b32 lookup_identifier(DeclarationInterner *decls_keys, DeclarationKey *mods, u32 mod_count, StringKey name, DeclarationKey *out);

void compiler_print_all_messages(Compiler *compiler);

b32 compile(Compiler *compiler);

b32 run_main(Compiler *compiler);

#endif // COMPILER_H
