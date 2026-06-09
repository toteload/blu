#ifndef STRING_INTERNER_H
#define STRING_INTERNER_H

#include "blu.h"

#define SEGMENTLIST_NAME StringList
#define SEGMENTLIST_TYPE String
#define SEGMENTLIST_MIN_SIZE_LOG2 6
#define SEGMENTLIST_SEGMENT_COUNT 24
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

#define HASHMAP_NAME       StringIndexMap
#define HASHMAP_KEY_TYPE   String
#define HASHMAP_VALUE_TYPE StringIndex
#define HASHMAP_OUTPUT_TYPES
#include "hashmap.h"

typedef struct {
  Arena          *arena;
  StringList      list;
  StringIndexMap  map;
} StringInterner;

typedef struct {
  Arena     *arena;
  Allocator  map_allocator;
} StringInternerOptions;

void        strings_init(StringInterner *strings, StringInternerOptions *options);
void        strings_deinit(StringInterner *strings);
StringIndex strings_add(StringInterner *strings, String s);
String      strings_get(StringInterner *strings, StringIndex idx);

#endif // STRING_INTERNER_H
