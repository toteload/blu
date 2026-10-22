#ifndef TYPES_H
#define TYPES_H

#include "blu.h"

typedef enum {
  Type_stub,
  Type_comptime_int,
  Type_usize, // pointer-width unsigned int
  Type_isize, // pointer-width signed int
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

  TypeKey t_stub;
  TypeKey t_comptime_int;
  TypeKey t_type;
  TypeKey t_nil;
  TypeKey t_bool;
  TypeKey t_never;
  TypeKey t_u8;
  TypeKey t_i8;
  TypeKey t_i16;
  TypeKey t_i32;
  TypeKey t_i64;
  TypeKey t_usize;
} TypeInterner;

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
TypeSizeInfo types_size_info_by_index(TypeInterner *types, TypeKey idx);

typedef enum {
  Coercion_invalid,
  Coercion_identical_types,
  Coercion_comptime_int_to_intlike,
  Coercion_int_widen,
  Coercion_bool_to_int,
  Coercion_array_to_slice,
} CoercionResult;

u32 classify_type_coercion(TypeInterner *types, TypeKey to, TypeKey from);

// An explicit cast. Allows everything a coercion allows, plus narrowing and signedness changes
// between integer types, which truncate.
b32 is_type_castable_to(TypeInterner *types, TypeKey to, TypeKey from);

b8 is_type_comptime(TypeInterner *types, TypeKey t);

// An add is always between two values of the same type.
b32 check_can_type_add(Type *t);

b32 check_can_type_cmp(Type *t);

String write_type(Arena *arena, TypeInterner *types, TypeKey type);

#endif // TYPES_H
