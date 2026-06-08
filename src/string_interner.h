#ifndef STRING_INTERNER_H
#define STRING_INTERNER_H

#include "toteload.h"

typedef u32 StringIndex;

#define VECTOR_NAME StringVector
#define VECTOR_TYPE String
#include "vector.h"

#define XXH_INLINE_ALL
#include "xxhash.h"

#define HASHMAP_NAME       StringIndexMap
#define HASHMAP_KEY_TYPE   String
#define HASHMAP_VALUE_TYPE StringIndex
#include "hash_map.h"

typedef struct StringInterner {
  Arena          *storage;
  StringIndexMap  map;
  StringVector    vec;
} StringInterner;

typedef struct StringInternerOptions {
  Arena     *string_storage;
  Allocator  map_allocator;
  Allocator  vec_allocator;
} StringInternerOptions;

void        string_interner_init(StringInterner *strings, StringInternerOptions *options);
void        string_interner_deinit(StringInterner *strings);
IndexString string_interner_add(StringInterner *strings, String s);
String      string_interner_get(StringInterner *strings, StringIndex idx);

#endif // STRING_INTERNER_H
