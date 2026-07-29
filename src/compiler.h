#ifndef COMPILER_H
#define COMPILER_H

#include "blu.h"
#include "messages.h"
#include "value.h"
#include "string_interner.h"
#include "ir.h"

typedef struct {
  struct {
    TypeIndex comptime_int;
    TypeIndex nil;
    TypeIndex type;
    TypeIndex i32;
  } type;

  struct {
    ValueIndex type;
    ValueIndex nil;
    ValueIndex i32;
  } val;
} Common;

typedef struct {
  DeclarationIndex parent;
  StringIndex      name;
} DeclarationKey;

typedef enum {
  ResolveStatus_unresolved,
  ResolveStatus_resolving_type,
  ResolveStatus_type_resolved,
  ResolveStatus_resolving_value,
  ResolveStatus_fully_resolved,
} ResolveStatus;

enum DeclarationKind {
  Declaration_root,
  Declaration_primitive,
  Declaration_mod,
  Declaration_decl,
};

typedef struct {
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
      u32      typecheck_end;
      IrChunk  chunk;
      ValueIndex val;
    } decl;
  } data;
} Declaration;

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

void compiler_init(Compiler *compiler);
void compiler_deinit(Compiler *compiler);

void compiler_add_sourcefile(Compiler *compiler, String filename);

b32 lookup_identifier(DeclarationInterner *decls_keys, DeclarationIndex *mods, u32 mod_count, StringIndex name, DeclarationIndex *out);

void compiler_print_all_messages(Compiler *compiler);

b32 compile(Compiler *compiler);

#endif // COMPILER_H
