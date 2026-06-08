#ifndef TYPES_H
#define TYPES_H

#include "toteload.h"

typedef u32 TypeIndex;

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

typedef struct {
  u32 size;
  u32 stride;
  u32 align;
} TypeSizeInfo;

typedef struct {
  u8 kind;

  union {
    struct {
      u8 signedness;
      u16 bitwidth;
    } integer;

    struct {
      TypeIndex base_type;
    } slice;

    struct {
      TypeIndex base_type;
      i64 size;
    } array;

    struct {
      TypeIndex return_type;
      u32 param_count;
      TypeIndex param_types[0];
    } function;
  } data;
} Type;

typedef Type *TypePtr;

#define ARRAYLIST_NAME TypeList
#define ARRAYLIST_TYPE TypePtr
#include "array_list.h"

#define HASHMAP_NAME       UniqueTypeMap
#define HASHMAP_KEY_TYPE   TypePtr
#define HASHMAP_VALUE_TYPE TypeIndex
#include "hash_map.h"

typedef struct {
  Arena *storage;
  UniqueTypeMap map;
  TypeList list;
} TypeInterner;

typedef struct {
  Arena *type_storage;
  Allocator map_allocator;
  Allocator list_allocator;
} TypeInternerOptions; 

void types_init(TypeInterner *types, TypeInternerOptions *options);
void types_deinit(TypeInterner *types);
TypeIndex types_add(TypeInterner *types, Type *type);
Type *types_get(TypeInterner *types, TypeIndex idx);

#endif // TYPES_H
