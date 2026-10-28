#ifndef VALUE_H
#define VALUE_H

#include "blu.h"
#include "types.h"
#include "resolver.h"

typedef struct {
  TypeKey  type;
  u32      data_size;
  void    *data;
} Value;

typedef struct {
  ValueKey val;
  u32 offset;
} ValuePointer;

typedef struct {
  usize  len;
  void  *data;
} ValueSlice;

typedef struct {
  ResidualFunctionKey key;
} ValueResidualFunction;

typedef struct {
  StubKey stub;
} ValueStubRef;

// -----------------------------------------------------------------------------

#define SEGMENTLIST_NAME          ValueList
#define SEGMENTLIST_TYPE          Value
#define SEGMENTLIST_MIN_SIZE_LOG2 6
#define SEGMENTLIST_SEGMENT_COUNT 24
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

struct ValueStore {
  Arena     *arena;
  Allocator  payload_allocator;
  ValueList  list;
};

typedef struct {
  Arena     *arena;
  Allocator  payload_allocator;
} ValueStoreOptions;

void values_init(ValueStore *values, ValueStoreOptions *options);
void values_deinit(ValueStore *values);

ValueKey  values_alloc(ValueStore *values, Value **out);
void     *values_alloc_data(ValueStore *values, u32 size, u32 align);
void      values_dealloc_data(ValueStore *values, void *data, u32 size);
void      values_dealloc(ValueStore *values, ValueKey idx);
Value    *values_get(ValueStore *values, ValueKey idx);
ValueKey  values_copy(ValueStore *values, ValueKey val);

#define values_alloc_data_type(values, type) \
  Cast(type*, values_alloc_data((values), sizeof(type), Align_of(type)))

#endif // VALUE_H
