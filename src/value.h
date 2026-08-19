#ifndef VALUE_H
#define VALUE_H

#include "blu.h"
#include "types.h"
#include "ir.h"

typedef struct {
  TypeIndex  type;
  u32        data_size;
  void      *data;
} Value;

#define SEGMENTLIST_NAME          ValueList
#define SEGMENTLIST_TYPE          Value
#define SEGMENTLIST_MIN_SIZE_LOG2 6
#define SEGMENTLIST_SEGMENT_COUNT 24
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

typedef struct {
  usize  len;
  void  *data;
} ValueSlice;

typedef struct {
  IrChunk chunk;
} ValueFunc;

typedef struct {
  ValueIndex val;
  u32 offset;
} ValuePointer;

// -----------------------------------------------------------------------------

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

ValueIndex  values_alloc(ValueStore *values, Value **out);
void       *values_alloc_data(ValueStore *values, u32 size, u32 align);
void        values_dealloc_data(ValueStore *values, void *data, u32 size);
void        values_dealloc(ValueStore *values, ValueIndex idx);
Value      *values_get(ValueStore *values, ValueIndex idx);
ValueIndex  values_copy(ValueStore *values, ValueIndex val);

#define values_alloc_data_type(values, type) \
  Cast(type*, values_alloc_data((values), sizeof(type), Align_of(type)))

#endif // VALUE_H
