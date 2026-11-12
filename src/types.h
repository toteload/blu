#ifndef TYPES_H
#define TYPES_H

#include "blu.h"

typedef enum {
  Type_stub,
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
  b8 is_uisize;
  u8 signedness;
  u16 bitwidth;
} TypeInteger;

typedef struct {
  TypeKey base_type;
} TypeSlice;

typedef struct {
  TypeKey base_type;
  u64 size;
} TypeArray;

typedef struct {
  TypeKey return_type;
  u32 param_count;
  TypeKey param_types[];
} TypeFunction;

typedef struct {
  TypeKey base_type;
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

#define INTERNER_NAME       TypeInternerAux
#define INTERNER_TYPE       TypePtr
#define INTERNER_INDEX_TYPE u32
#define INTERNER_OUTPUT_TYPES
#include "interner.h"

typedef struct {
  TypeInternerAux _interner;
} TypeInterner;

#define TYPE_STUB         ((TypeKey){1})
#define TYPE_TYPE         ((TypeKey){2})
#define TYPE_NIL          ((TypeKey){3})
#define TYPE_BOOL         ((TypeKey){4})
#define TYPE_NEVER        ((TypeKey){5})
#define TYPE_COMPTIME_INT ((TypeKey){6})
#define TYPE_U8           ((TypeKey){7})
#define TYPE_U16          ((TypeKey){8})
#define TYPE_U32          ((TypeKey){9})
#define TYPE_U64          ((TypeKey){10})
#define TYPE_USIZE        ((TypeKey){11})
#define TYPE_I8           ((TypeKey){12})
#define TYPE_I16          ((TypeKey){13})
#define TYPE_I32          ((TypeKey){14})
#define TYPE_I64          ((TypeKey){15})
#define TYPE_ISIZE        ((TypeKey){16})

void types_init(TypeInterner *types, InternerOptions *options);
void types_deinit(TypeInterner *types);

TypeKey types_intern(TypeInterner *types, Type *t);
Type *types_get(TypeInterner *types, TypeKey key);

TypeKey types_add_pointer(TypeInterner *types, TypeKey base_type);

#define arena_push_type_function(arena,param_count) arena_push(arena, sizeof(Type) + (param_count) *sizeof(TypeKey), Align_of(Type))

// Types are variable in size. This functions returns the actual size in bytes for a given type.
// This is NOT the runtime size. For example, if you pass an array type to this funcion it will
// return a constant size, since all you need to store for the type of an array is the base type and its size.
u32 type_intern_byte_size(Type *type);

TypeSizeInfo types_size_info(TypeInterner *types, Type *type);
TypeSizeInfo types_size_info_by_key(TypeInterner *types, TypeKey idx);

typedef enum {
  Coercion_invalid,
  Coercion_identical_types,
  Coercion_int_widen,
  Coercion_bool_to_int,
  Coercion_array_to_slice,
  Coercion_comptime,
} CoercionResult;

u32 classify_type_coercion(TypeInterner *types, TypeKey to, TypeKey from);

typedef enum {
  CAST_INVALID,
  CAST_COERCION_IDENTICAL_TYPES,
  CAST_COERCION_INT_WIDEN,
  CAST_COERCION_BOOL_TO_INT,
  CAST_COERCION_ARRAY_TO_SLICE,
  CAST_COERCION_COMPTIME,
  CAST_INT_RANGE_CHECK,
} CastClassification;

TTLD_STATIC_ASSERT(Coercion_invalid         == CAST_INVALID,                  "Coercion classification must be in sync with cast classification");
TTLD_STATIC_ASSERT(Coercion_identical_types == CAST_COERCION_IDENTICAL_TYPES, "Coercion classification must be in sync with cast classification");
TTLD_STATIC_ASSERT(Coercion_int_widen       == CAST_COERCION_INT_WIDEN,       "Coercion classification must be in sync with cast classification");
TTLD_STATIC_ASSERT(Coercion_bool_to_int     == CAST_COERCION_BOOL_TO_INT,     "Coercion classification must be in sync with cast classification");
TTLD_STATIC_ASSERT(Coercion_array_to_slice  == CAST_COERCION_ARRAY_TO_SLICE,  "Coercion classification must be in sync with cast classification");
TTLD_STATIC_ASSERT(Coercion_comptime        == CAST_COERCION_COMPTIME,        "Coercion classification must be in sync with cast classification");

u32 classify_type_cast(TypeInterner *types, TypeKey to, TypeKey from);

b8 is_type_comptime(TypeInterner *types, TypeKey t);

// An add is always between two values of the same type.
b32 check_type_for_op_bitshift(TypeKey lhs, TypeKey rhs);
b32 check_type_for_op_bitwise(TypeKey t);
b32 check_type_for_op_arithmetic(TypeKey t);
b32 check_type_for_op_cmp(TypeKey t);
b32 check_type_for_op_negate(TypeKey t);

String write_type(Arena *arena, TypeInterner *types, TypeKey type);

#endif // TYPES_H
