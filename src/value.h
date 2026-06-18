#ifndef VALUE_H
#define VALUE_H

#include "blu.h"

typedef struct {
  TypeIndex  type;
  void      *data;
} Value;

struct ValueStore {
  Arena     arena;
  Allocator payload_allocator;
};

typedef struct {
  Allocator payload_allocator;
} ValueStoreOptions;

void values_init(ValueStore *values, ValueStoreOptions *options);
void values_deinit(ValueStore *values);

ValueIndex  values_alloc(ValueStore *values, Value **out);
Value      *values_get(ValueStore *values, ValueIndex idx);

#endif // VALUE_H
