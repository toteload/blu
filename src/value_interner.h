#ifndef VALUE_INTERNER_H
#define VALUE_INTERNER_H

#include "blu.h"
#include "types.h"

typedef u32 ValueOffset;

#define BITMASK_VREF_IS_OFFSET (Cast(u32, 1) << 31)

typedef struct { u32 x; } ValueRef;

always_inline b32 vref_is_value(ValueRef r) {
  return (r.x & BITMASK_VREF_IS_OFFSET) == 0;
}

always_inline b32 vref_is_some_value(ValueRef r) {
  return r.x != 0 && vref_is_value(r);
}

always_inline b32 vref_is_offset(ValueRef r) {
  return (r.x & BITMASK_VREF_IS_OFFSET) != 0;
}

always_inline b32 vref_is_nil(ValueRef r) {
  return r.x == 0;
}

always_inline ValueKey vref_to_value(ValueRef r) {
  ASSERT_SAFE(vref_is_value(r));
  return (ValueKey){r.x};
}

always_inline ValueOffset vref_to_offset(ValueRef r) {
  ASSERT_SAFE(vref_is_offset(r));
  return r.x & ~BITMASK_VREF_IS_OFFSET;
}

always_inline ValueRef vref_from_offset(ValueOffset offset) {
  return (ValueRef){offset | BITMASK_VREF_IS_OFFSET};
}

always_inline ValueRef vref_from_value(ValueKey key) {
  return (ValueRef){key.x};
}

always_inline u32 vref_to_u32(ValueRef r) {
  return r.x;
}

#define VREF_NIL ((ValueRef){0})

enum {
  VAL_TYPE_TYPE = 1,
  VAL_TYPE_COMPTIME_INT,
  VAL_TYPE_NEVER,
  VAL_TYPE_BOOL,
  VAL_TYPE_I8,
  VAL_TYPE_I16,
  VAL_TYPE_I32,
  VAL_TYPE_I64,
  VAL_TYPE_U8,
  VAL_TYPE_U16,
  VAL_TYPE_U32,
  VAL_TYPE_U64,
  VAL_TYPE_USIZE,
  VAL_TYPE_ISIZE,

  VAL_BOOL_TRUE,
  VAL_BOOL_FALSE,
  VAL_COMPTIME_INT_1,
};

typedef struct {
  TypeKey type;
  u32 size; // size of this value = header + data
  _Alignas(8) u8 data[];
} Value;

typedef struct {
  ValueKey val;
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
ValueKey values_intern_data(ValueInterner *values, TypeKey type, void const *data, u32 data_size);
Value *values_get(ValueInterner *values, ValueKey key);

#endif // VALUE_INTERNER_H
