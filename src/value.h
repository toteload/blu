#ifndef VALUE_H
#define VALUE_H

#include "blu.h"

#define SEGMENTLIST_NAME          ValueList
#define SEGMENTLIST_TYPE          Value
#define SEGMENTLIST_MIN_SIZE_LOG2 6
#define SEGMENTLIST_SEGMENT_COUNT 24
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

typedef struct {
  TypeIndex  type;
  void      *data;
} Value;

typedef struct {
  usize  len;
  void  *data;
} ValueSlice;

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
void        values_dealloc(ValueStore *values, ValueIndex idx, TypeSizeInfo size_info);
Value      *values_get(ValueStore *values, ValueIndex idx);

#endif // VALUE_H
