#ifndef TYPES_H
#define TYPES_H

#include "blu.h"

enum TypeKind {
  Type_integer,
  Type_boolean,
  Type_function,
  Type_nil,
  Type_never,
  Type_slice,
  Type_array,
  Type_type,
};

enum Signedness {
  Unsigned,
  Signed,
};

enum TypeAttribute {
  TypeAttribute_comptime = 1 << 0,
};

typedef struct {
  u32 size;
  u32 stride;
  u32 align;
} TypeSizeInfo;

typedef struct {
  u8 signedness;
  u16 bitwidth;
} TypeInteger;

typedef struct {
  TypeIndex base_type;
} TypeSlice;

typedef struct {
  TypeIndex base_type;
  u64 size;
} TypeArray;

typedef struct {
  u8 kind;
  u8 attributes;

  union {
    TypeInteger integer;
    TypeSlice   slice;
    TypeArray   array;

    struct {
      TypeIndex return_type;
      u32 param_count;
    } function;
  } data;

  // The compiler complains that you are not allowed to put flexible array members (FAM) in nested 
  // structs, when I put the FAM inside the `function` struct. I don't think it should be a problem, 
  // but I'll listen to the compiler.
  TypeIndex function_param_types[];
} Type;

typedef Type *TypePtr;

#define SEGMENTLIST_NAME TypeList
#define SEGMENTLIST_TYPE TypePtr
#define SEGMENTLIST_MIN_SIZE_LOG2 6
#define SEGMENTLIST_SEGMENT_COUNT 24
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

#define HASHMAP_NAME       UniqueTypeMap
#define HASHMAP_KEY_TYPE   TypePtr
#define HASHMAP_VALUE_TYPE TypeIndex
#define HASHMAP_OUTPUT_TYPES
#include "hashmap.h"

struct TypeInterner {
  Arena         arena;
  TypeList      list;
  UniqueTypeMap map;
};

typedef struct {
  Allocator map_allocator;
  Arena *scratch;
} TypeInternerOptions; 

void       types_init(TypeInterner *types, TypeInternerOptions *options);
void       types_deinit(TypeInterner *types);
TypeIndex  types_add(TypeInterner *types, Type *type);
Type      *types_get(TypeInterner *types, TypeIndex idx);

TypeSizeInfo types_size_info_by_type(TypeInterner *types, Type *type);
TypeSizeInfo types_size_info(TypeInterner *types, TypeIndex idx);

#endif // TYPES_H
