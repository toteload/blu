#ifndef VALUE_H
#define VALUE_H

#include "blu.h"

typedef struct {
  TypeIndex type;
  void *data;
} Value;

typedef struct {
} ValueStore;

void values_init(ValueStore *values);
void values_deinit(ValueStore *values);

ValueIndex values_alloc(ValueStore *values, Value **out);

#endif // VALUE_H
