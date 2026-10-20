#include "symbol_map.h"

// https://github.com/skeeto/hash-prospector/issues/19
// score: 0.10734781817103507
internal u32 lowbias32(u32 x) {
  x ^= x >> 16;
  x *= 0x21f0aaad;
  x ^= x >> 15;
  x *= 0xf35a2d97;
  x ^= x >> 15;

  return x;
}

internal u32 hash_string_index(void *context, StringIndex s) {
  Unused(context);
  return lowbias32(s);
}

internal b32 cmp_string_index(void *context, StringIndex a, StringIndex b) {
  Unused(context);
  return a == b;
}

#define HASHMAP_NAME            LocalsMap
#define HASHMAP_KEY_TYPE        StringIndex
#define HASHMAP_VALUE_TYPE      u32
#define HASHMAP_FUNCTION_PREFIX locals
#define HASHMAP_HASH_FN         hash_string_index
#define HASHMAP_KEY_COMPARE_FN  cmp_string_index
#define HASHMAP_OUTPUT_DEFINITIONS
#include "hashmap.h"

#define SEGMENTLIST_NAME SCOPE_ENTRY_STACK_NAME
#define SEGMENTLIST_TYPE SCOPE_ENTRY_STACK_TYPE
#define SEGMENTLIST_MIN_SIZE_LOG2 SCOPE_ENTRY_STACK_MIN_SIZE_LOG2
#define SEGMENTLIST_SEGMENT_COUNT SCOPE_ENTRY_STACK_SEGMENT_COUNT
#define SEGMENTLIST_LINKAGE internal
#define SEGMENTLIST_FUNCTION_PREFIX scopes
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

void symbols_init(SymbolMap *symbols, Allocator hashmap_allocator, Arena *scratch) {
  *symbols = (SymbolMap){ .scratch = scratch, };

  locals_init(&symbols->locals, &(HashMapOptions){
    .allocator = hashmap_allocator,
    .initial_size = 32,
  });

  scopes_append(&symbols->stack, scratch, (ScopeEntry){ .kind = ScopeEntry_root });
}

void symbols_deinit(SymbolMap *symbols) {
  locals_deinit(&symbols->locals);
}

internal b32 add_symbol(SymbolMap *syms, StringIndex name, u32 idx, u32 *occupied_idx) {
  b32 was_occupied = False;
  LocalsMapBucket *bucket = locals_insert_key_and_get_bucket(&syms->locals, name, &was_occupied);
  if (was_occupied) {
    u32 idx = bucket->val;
    ScopeEntry *existing = scopes_ptr_at_unchecked(&syms->stack, idx);

    // Identical symbol found in a parent scope.
    if (existing->parent_block_idx != syms->parent_block_idx) {
      *occupied_idx = idx;
      return False;
    }
  }

  bucket->val = idx;

  return True;
}

SymbolsPushScopeResult symbols_push_scope(SymbolMap *syms, ScopeEntryKind kind, InstructionIndex inst, StringIndex label) {
  u32 offset = syms->stack.len;

  scopes_append(&syms->stack, syms->scratch, (ScopeEntry){
    .kind = kind,
    .parent_loop_idx = syms->parent_loop_idx,
    .parent_block_idx = syms->parent_block_idx,
    .name = label,
    .inst = inst,
  });

  syms->parent_block_idx = offset;

  if (kind == ScopeEntry_loop) {
    syms->parent_loop_idx = offset;
  }

  u32 occupied_idx;
  if (label && !add_symbol(syms, label, offset, &occupied_idx)) {
    ScopeEntry *existing = scopes_ptr_at_unchecked(&syms->stack, occupied_idx);
    return (SymbolsPushScopeResult){
      .err = SYMBOLS_ERR_FOUND_EXISTING_SYMBOL,
      .data.inst = existing->inst,
    };
  }

  return (SymbolsPushScopeResult){
    .err = SYMBOLS_OK,
    .data.scope_key = { offset },
  };
}

SymbolsPushLocalResult symbols_push_local(SymbolMap *syms, ScopeEntryKind kind, StringIndex name, InstructionIndex inst) {
  u32 offset = syms->stack.len;

  u32 occupied_idx;
  if (!add_symbol(syms, name, offset, &occupied_idx)) {
    ScopeEntry *existing = scopes_ptr_at_unchecked(&syms->stack, occupied_idx);
    return (SymbolsPushLocalResult){
      .err = SYMBOLS_ERR_FOUND_EXISTING_SYMBOL,
      .inst = existing->inst,
    };
  }

  scopes_append(&syms->stack, syms->scratch, (ScopeEntry){
    .kind = kind,
    .parent_loop_idx = syms->parent_loop_idx,
    .parent_block_idx = syms->parent_block_idx,
    .name = name,
    .inst = inst,
  });

  return (SymbolsPushLocalResult){
    .err = SYMBOLS_OK,
  };
}

void symbols_pop_scope(SymbolMap *syms, ScopeKey key) {
  while (syms->stack.len < key.offset - 1) {
    ScopeEntry entry = scopes_pop(&syms->stack);

    switch (Cast(ScopeEntryKind, entry.kind)) {
      case ScopeEntry_local: {
        locals_remove(&syms->locals, entry.name);
      } break;
      case ScopeEntry_block:
      case ScopeEntry_loop: {
        if (entry.name) {
          locals_remove(&syms->locals, entry.name);
        }
      } break;
      case ScopeEntry_root:
      case ScopeEntry_function:
      case ScopeEntry_param:
        Panic();
    }
  }

  ScopeEntry entry = scopes_pop(&syms->stack);

  syms->parent_loop_idx = entry.parent_loop_idx;
  syms->parent_block_idx = entry.parent_block_idx;
}

ScopeEntry *symbols_find(SymbolMap *syms, StringIndex name) {
  u32 *idx = locals_find(&syms->locals, name);
  if (is_null(idx)) {
    return Null;
  }

  return scopes_ptr_at_unchecked(&syms->stack, *idx);
}

ScopeEntry *symbols_get_enclosing_loop(SymbolMap *syms) {
  if (!syms->parent_loop_idx) {
    return Null;
  }

  return scopes_ptr_at_unchecked(&syms->stack, syms->parent_loop_idx);
}

