#ifndef VALUE_INTERNER_H
#define VALUE_INTERNER_H

#include "blu.h"
#include "types.h"

#define VAL_TYPE_TYPE         ((ValueKey){1})
#define VAL_TYPE_COMPTIME_INT ((ValueKey){2})
#define VAL_TYPE_NEVER        ((ValueKey){3})
#define VAL_TYPE_BOOL         ((ValueKey){4})
#define VAL_TYPE_I8           ((ValueKey){5})
#define VAL_TYPE_I16          ((ValueKey){6})
#define VAL_TYPE_I32          ((ValueKey){7})
#define VAL_TYPE_I64          ((ValueKey){8})
#define VAL_TYPE_U8           ((ValueKey){9})
#define VAL_TYPE_U16          ((ValueKey){10})
#define VAL_TYPE_U32          ((ValueKey){11})
#define VAL_TYPE_U64          ((ValueKey){12})
#define VAL_TYPE_USIZE        ((ValueKey){13})
#define VAL_TYPE_ISIZE        ((ValueKey){14})
#define VAL_TYPE_NIL          ((ValueKey){15})
#define VAL_BOOL_TRUE         ((ValueKey){16})
#define VAL_BOOL_FALSE        ((ValueKey){17})
#define VAL_COMPTIME_INT_1    ((ValueKey){18})
#define VAL_NIL_NIL           ((ValueKey){19})

typedef struct {
  TypeKey type;
  u32 size; // size of this value = header + data
  _Alignas(8) u8 data[];
} Value;

typedef struct { u32 x; } ValueRef;

always_inline b32 vref_is_value(ValueRef r) {
  return (r.x & REF_TAG_MASK) == REF_TAG_VALUE;
}

always_inline b32 vref_is_offset(ValueRef r) {
  return (r.x & REF_TAG_MASK) == REF_TAG_OFFSET;
}

always_inline ValueKey vref_to_value(ValueRef r) {
  ttld_assert_safe(vref_is_value(r));
  return (ValueKey){r.x};
}

always_inline ValueOffset vref_to_offset(ValueRef r) {
  ttld_assert_safe(vref_is_offset(r));
  return r.x & ~REF_TAG_MASK;
}

always_inline ValueRef vref_from_value(ValueKey key) {
  ttld_assert_safe((key.x & REF_TAG_MASK) == 0);
  return (ValueRef){key.x};
}

always_inline ValueRef vref_from_offset(ValueOffset offset) {
  ttld_assert_safe((offset & REF_TAG_MASK) == 0);
  return (ValueRef){offset | REF_TAG_OFFSET};
}

typedef struct {
  ValueRef base;
  u32      offset;
} ValuePointer;

typedef struct {
  usize        len;
  ValuePointer ptr;
} ValueSlice;

typedef struct {
  ResidualFunctionKey key;
} ValueResidualFunction;

typedef struct {
  StubKey stub;
} ValueStubRef;

typedef Value* ValuePtr;

#define INTERNER_NAME            AuxValueInterner
#define INTERNER_TYPE            ValuePtr
#define INTERNER_INDEX_TYPE      ValueKey
#define INTERNER_INDEX_IS_KEY
#define INTERNER_OUTPUT_TYPES
#include "interner.h"

typedef struct {
  TypeInterner *types;
  Arena *scratch;
} ValueInternerContext;

typedef struct ValueInterner {
  AuxValueInterner aux;
  ValueInternerContext context;
} ValueInterner;

typedef struct {
  Arena *storage;
  Allocator map_allocator;
  Arena *scratch;
  TypeInterner *types;
} ValueInternerInitOptions;

void values_init(ValueInterner *values, ValueInternerInitOptions *options);
void values_deinit(ValueInterner *values);

ValueKey values_intern(ValueInterner *values, Value *v);
ValueKey values_intern_data(ValueInterner *values, TypeKey type, void const *data);
Value *values_get(ValueInterner *values, ValueKey key);

#endif // VALUE_INTERNER_H
