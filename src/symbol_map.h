#ifndef SYMBOL_MAP_H
#define SYMBOL_MAP_H

#include "blu.h"

typedef enum {
  ScopeEntry_root,
  ScopeEntry_block,
  ScopeEntry_loop,
  ScopeEntry_function,
  ScopeEntry_local,
  ScopeEntry_param,
} ScopeEntryKind;

typedef struct {
  u8 kind;
  u32 parent_loop_idx;
  u32 parent_block_idx;
  StringKey name;
  InstructionOffset inst;
} ScopeEntry;

#define SCOPE_ENTRY_STACK_NAME ScopeEntryStack
#define SCOPE_ENTRY_STACK_TYPE ScopeEntry
#define SCOPE_ENTRY_STACK_MIN_SIZE_LOG2 8
#define SCOPE_ENTRY_STACK_SEGMENT_COUNT 16
#define SEGMENTLIST_NAME SCOPE_ENTRY_STACK_NAME
#define SEGMENTLIST_TYPE SCOPE_ENTRY_STACK_TYPE
#define SEGMENTLIST_MIN_SIZE_LOG2 SCOPE_ENTRY_STACK_MIN_SIZE_LOG2
#define SEGMENTLIST_SEGMENT_COUNT SCOPE_ENTRY_STACK_SEGMENT_COUNT
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

#define HASHMAP_NAME            LocalsMap
#define HASHMAP_KEY_TYPE        StringKey
#define HASHMAP_VALUE_TYPE      u32
#define HASHMAP_FUNCTION_PREFIX locals
#define HASHMAP_OUTPUT_TYPES
#include "hashmap.h"

typedef struct {
  LocalsMap locals;

  u32 parent_loop_idx;
  u32 parent_block_idx;

  Arena *scratch;
  ScopeEntryStack stack;
} SymbolMap;

typedef struct {
  u32 offset;
} ScopeKey;

enum {
  SYMBOLS_OK,
  SYMBOLS_ERR_FOUND_EXISTING_SYMBOL,
};

void symbols_init(SymbolMap *syms, Allocator hashmap_allocator, Arena *scratch);
void symbols_deinit(SymbolMap *syms);

typedef struct {
  u32 err;
  union {
    ScopeKey scope_key;
    InstructionOffset inst;
  } data;
} SymbolsPushScopeResult;

// - Fails if you push a scope with a label that is already in the symbol table.
// - kind should be block, loop, or function
SymbolsPushScopeResult symbols_push_scope(SymbolMap *syms, ScopeEntryKind kind, InstructionOffset inst, StringKey label);

typedef struct {
  u32 err;
  InstructionOffset inst;
} SymbolsPushLocalResult;

// - Fails if you add a name that is already in the symbol table.
// - kind should be param or local
SymbolsPushLocalResult symbols_push_local(SymbolMap *syms, ScopeEntryKind kind, StringKey name, InstructionOffset inst);

void symbols_pop_scope(SymbolMap *syms, ScopeKey key);

ScopeEntry *symbols_find(SymbolMap *syms, StringKey name);
ScopeEntry *symbols_get_enclosing_loop(SymbolMap *syms);

#endif // SYMBOL_MAP_H

