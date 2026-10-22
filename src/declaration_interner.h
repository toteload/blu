#ifndef DECLARATION_INTERNER_H
#define DECLARATION_INTERNER_H

#include "blu.h"
#include "source_file.h"
#include "ir.h"

typedef struct {
  DeclarationKey parent;
  StringKey      name;
} DeclarationPath;

typedef enum {
  Declaration_root,
  Declaration_primitive,
  Declaration_mod,
  Declaration_decl,
} DeclarationKind;

struct Declaration {
  DeclarationKey idx;

  u8 kind;

  union {
    ValueKey primitive;

    struct {
      Source *source;
      u32 tree_idx;
    } mod;

    struct {
      Source *source;
      u32 tree_idx;
      SIrChunk chunk;
      ValueKey val;
    } decl;
  } data;
};

#define DECLARATION_INTERNER_NAME DeclarationInterner
#define DECLARATION_INTERNER_TYPE DeclarationPath
#define DECLARATION_INTERNER_INDEX_TYPE DeclarationKey
#define DECLARATION_INTERNER_EXTRA_TYPE Declaration
#define DECLARATION_INTERNER_FUNCTION_PREFIX decls

#define INTERNER_NAME DECLARATION_INTERNER_NAME
#define INTERNER_TYPE DECLARATION_INTERNER_TYPE
#define INTERNER_INDEX_TYPE DECLARATION_INTERNER_INDEX_TYPE
#define INTERNER_EXTRA_TYPE DECLARATION_INTERNER_EXTRA_TYPE
#define INTERNER_FUNCTION_PREFIX DECLARATION_INTERNER_FUNCTION_PREFIX
#define INTERNER_OUTPUT_TYPES
#define INTERNER_OUTPUT_DECLARATIONS
#include "interner.h"

#ifdef DECLARATION_INTERNER_IMPLEMENTATION

#define XXH_INLINE_ALL
#include "xxhash.h"

internal u32 hash_decl_path(void *context, DeclarationPath key) {
  Unused(context);
  return XXH32(&key, sizeof(DeclarationPath), 0);
}

internal b32 cmp_decl_path(void *context, DeclarationPath a, DeclarationPath b) {
  Unused(context);
  return key_eq(a.parent, b.parent) && key_eq(a.name, b.name);
}

#define INTERNER_NAME DECLARATION_INTERNER_NAME
#define INTERNER_TYPE DECLARATION_INTERNER_TYPE
#define INTERNER_INDEX_TYPE DECLARATION_INTERNER_INDEX_TYPE
#define INTERNER_EXTRA_TYPE DECLARATION_INTERNER_EXTRA_TYPE
#define INTERNER_FUNCTION_PREFIX DECLARATION_INTERNER_FUNCTION_PREFIX
#define INTERNER_RESERVE_ZERO_INDEX
#define INTERNER_INDEX_IS_KEY
#define INTERNER_HASH_FN hash_decl_path
#define INTERNER_COMPARE_FN cmp_decl_path
#define INTERNER_OUTPUT_DEFINITIONS
#include "interner.h"

#endif // DECLARATION_INTERNER_IMPLEMENTATION

#endif // DECLARATION_INTERNER_H
