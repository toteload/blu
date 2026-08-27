#ifndef COMPILER_H
#define COMPILER_H

#include "blu.h"
#include "messages.h"
#include "string_interner.h"
#include "value.h"
#include "ir.h"
#include "cli_options.h"

typedef struct {
  struct {
    TypeIndex comptime_int;
    TypeIndex type;
    TypeIndex nil;
    TypeIndex bool;
    TypeIndex never;
    TypeIndex u8;
    TypeIndex i8;
    TypeIndex i16;
    TypeIndex i32;
    TypeIndex i64;
    TypeIndex usize;
  } type;

  struct {
    ValueIndex type;
    ValueIndex nil;
    ValueIndex bool;
    ValueIndex never;
    ValueIndex i8;
    ValueIndex i16;
    ValueIndex i32;
    ValueIndex i64;
    ValueIndex u8;
    ValueIndex comptime_int;
    ValueIndex usize;

    ValueIndex true;
    ValueIndex false;
  } val;
} Common;

typedef struct {
  DeclarationIndex parent;
  StringIndex      name;
} DeclarationKey;

typedef enum {
  ResolveStatus_error,
  ResolveStatus_unresolved,
  ResolveStatus_resolving_type,
  ResolveStatus_type_resolved,
  ResolveStatus_resolving_value,
  ResolveStatus_fully_resolved,
} ResolveStatus;

typedef enum {
  Declaration_root,
  Declaration_primitive,
  Declaration_mod,
  Declaration_decl,
} DeclarationKind;

struct Declaration {
  DeclarationIndex idx;

  u8 kind;
  u8 resolve_status;

  union {
    ValueIndex primitive;

    struct {
      Source *source;
      u32     tree_idx;
    } mod;

    struct {
      Source  *source;
      u32      tree_idx;

      SIrChunk  chunk;

      InstructionIndex block_type;
      TypeIndex type; // will be set after the type of the declaration has been resolved

      InstructionIndex block_val;
      ValueIndex val; // will be set after the declaration has been fully resolved
    } decl;
  } data;
};

#define INTERNER_NAME       DeclarationInterner
#define INTERNER_TYPE       DeclarationKey
#define INTERNER_INDEX_TYPE DeclarationIndex
#define INTERNER_EXTRA_TYPE Declaration
#define INTERNER_FUNCTION_PREFIX decls
#define INTERNER_OUTPUT_TYPES
#define INTERNER_OUTPUT_DECLARATIONS
#include "interner.h"

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
