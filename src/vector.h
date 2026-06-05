#ifndef VECTOR_TYPE
#endif

#ifndef VECTOR_NAME_PREFIX
#endif

#include "toteload.h"

typedef struct Cat(VECTOR_NAME_PREFIX, _Vector) {
  usize len;
  usize cap;
  VECTOR_TYPE *data;
} Cat(VECTOR_NAME_PREFIX, _Vector);

b32  Cat(VECTOR_NAME_PREFIX, _is_empty)(Cat(VECTOR_NAME_PREFIX, _Vector) *v);
void Cat(VECTOR_NAME_PREFIX, _push)(Cat(VECTOR_NAME_PREFIX, _Vector) *v, VECTOR_TYPE x);

#ifdef VECTOR_IMPLEMENTATION
#endif
