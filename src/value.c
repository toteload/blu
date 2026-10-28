#include "value.h"

#define SEGMENTLIST_NAME            ValueList
#define SEGMENTLIST_TYPE            Value
#define SEGMENTLIST_MIN_SIZE_LOG2   6
#define SEGMENTLIST_SEGMENT_COUNT   24
#define SEGMENTLIST_FUNCTION_PREFIX list
#define SEGMENTLIST_LINKAGE         internal
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

void values_init(ValueStore *values, ValueStoreOptions *options) {
  *values = (ValueStore){
    .arena             = options->arena,
    .payload_allocator = options->payload_allocator,
    .list              = {0},
  };
  list_push(&values->list, values->arena); // Reserve nil entry
}

ValueKey values_alloc(ValueStore *values, Value **out) {
  u32 idx = values->list.len;
  *out = list_push(&values->list, values->arena);
  return (ValueKey){idx};
}

void *values_alloc_data(ValueStore *values, u32 size, u32 align) {
  return Alloc(values->payload_allocator, size, align);
}

void values_dealloc_data(ValueStore *values, void *data, u32 size) {
  Free(values->payload_allocator, data, size);
}

void values_dealloc(ValueStore *values, ValueKey idx) {
  Value *p = values_get(values, idx);
  if (p->data_size > 0) {
    Free(values->payload_allocator, p->data, p->data_size);
  }
}

Value *values_get(ValueStore *values, ValueKey idx) {
  return list_ptr_at_unchecked(&values->list, idx.x);
}

ValueKey values_copy(ValueStore *values, ValueKey val) {
  Value *v;
  ValueKey res = values_alloc(values, &v);
  {
    Value *s = values_get(values, val);
    u32 align = 16; // TODO: not the nicest solution as this could be too small.
    void *data = values_alloc_data(values, s->data_size, align);
    memcpy(data, s->data, s->data_size);
    *v = (Value){
      .type = s->type,
      .data = data,
      .data_size = s->data_size,
    };
  }
  return res;
}

