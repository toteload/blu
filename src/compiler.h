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
#define SEGMENTLIST_TYPE          DeclarationIndex
#define SEGMENTLIST_MIN_SIZE_LOG2 DECLIDXLIST_MIN_SIZE_LOG2
#define SEGMENTLIST_SEGMENT_COUNT DECLIDXLIST_SEGMENT_COUNT
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"



typedef struct {
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
} Compiler;

void compiler_init(Compiler *compiler, CLIOptions *options);
void compiler_deinit(Compiler *compiler);

void compiler_add_sourcefile(Compiler *compiler, String filename);
Source *compiler_get_source(Compiler *compiler, SourceIndex source_idx);

b32 lookup_identifier(DeclarationInterner *decls_keys, DeclarationIndex *mods, u32 mod_count, StringIndex name, DeclarationIndex *out);

void compiler_print_all_messages(Compiler *compiler);

b32 compile(Compiler *compiler);

b32 run_main(Compiler *compiler);

#endif // COMPILER_H
