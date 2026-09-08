#include "blu.h"

typedef struct {
  TypeKey type;
  u32     data_size;
  u8      data[];
} Value;

typedef struct {
  ValueKey val;
  u32      offset;
} ValuePointer;

typedef struct {
  usize        len;
  ValuePointer ptr;
} ValueSlice;

