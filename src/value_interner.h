#ifndef VALUE_INTERNER_H
#define VALUE_INTERNER_H

#include "blu.h"
#include "types.h"

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

typedef struct {
  AuxValueInterner aux;
  ValueInternerContext context;
} ValueInterner;

typedef struct {
  Arena *storage;
  Allocator map_allocator;
  Arena *scratch;
  TypeInterner *types;
} ValueInternerInitOptions;

void values2_init(ValueInterner *values, ValueInternerInitOptions *options);
void values2_deinit(ValueInterner *values);

ValueKey values2_intern(ValueInterner *values, Value *v);
Value *values2_get(ValueInterner *values, ValueKey key);

#endif // VALUE_INTERNER_H
