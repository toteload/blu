#include "string_interner.h"

#define XXH_INLINE_ALL
#include "xxhash.h"

u32 str_hash(void *context, String s) { 
  Unused(context);
  return XXH32(s.str, s.len, 0);
}

b32 str_eq(void *context, String a, String b) {
  Unused(context);
  return string_eq(a, b);
}

#define INTERNER_NAME            StringInterner
#define INTERNER_TYPE            String
#define INTERNER_INDEX_TYPE      StringIndex
#define INTERNER_FUNCTION_PREFIX strings
#define INTERNER_HASH_FN         str_hash
#define INTERNER_COMPARE_FN      str_eq
#define INTERNER_COPY_FN         arena_copy_string
#define INTERNER_OUTPUT_DEFINITIONS
#include "interner.h"
