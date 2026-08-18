#ifndef TYPES_H
#define TYPES_H

#include "blu.h"

typedef enum {
  Type_comptime_int,
  Type_integer,
  Type_bool,
  Type_function,
  Type_nil,
  Type_never,
  Type_slice,
  Type_array,
  Type_type,
  Type_pointer,
} TypeKind;

// How a comptime_int is actually stored. This will probably grow at some point.
typedef i64 ComptimeInt;

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
  TypeIndex return_type;
  u32 param_count;
  TypeIndex param_types[];
} TypeFunction;

typedef struct {
  TypeIndex base_type;
} TypePointer;

typedef struct {
  u8 kind;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wflexible-array-extensions"
  union {
    TypeInteger  integer;
    TypeSlice    slice;
    TypeArray    array;
    TypeFunction function;
    TypePointer  pointer;
  } data;
#pragma clang diagnostic pop
} Type;

typedef Type *TypePtr;

#define INTERNER_NAME            TypeInterner
#define INTERNER_TYPE            TypePtr
#define INTERNER_INDEX_TYPE      TypeIndex
#define INTERNER_FUNCTION_PREFIX types
#define INTERNER_OUTPUT_TYPES
#define INTERNER_OUTPUT_DECLARATIONS
#include "interner.h"

#define arena_push_type_function(arena,param_count) arena_push(arena, sizeof(Type) + (param_count) *sizeof(TypeIndex), Align_of(Type))

// Types are variable in size. This functions returns the actual size in bytes for a given type.
// This is NOT the runtime size. For example, if you pass an array type to this funcion it will
// return a constant size, since all you need to store for an array is the base type and its size.
u32 type_intern_byte_size(Type *type);

TypeSizeInfo types_size_info(TypeInterner *types, Type *type);
TypeSizeInfo types_size_info_by_index(TypeInterner *types, TypeIndex idx);

b32 is_type_coercible_to(TypeInterner *types, TypeIndex to, TypeIndex from);

#endif // TYPES_H
