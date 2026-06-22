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
}

ValueIndex values_alloc(ValueStore *values, Value **out) {
  u32 idx = values->list.len;
  *out = list_push(&values->list, values->arena);
  return idx;
}

void values_dealloc(ValueStore *values, ValueIndex idx) {
  Value *p = values_get(values, idx);
  if (p->data) {
    values_dealloc_data(values, p->data, );
  }
}

Value *values_get(ValueStore *values, ValueIndex idx) {
  return list_ptr_at_unchecked(&values->list, idx);
}
