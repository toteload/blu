#ifndef VECTOR_TYPE
#endif

#ifndef VECTOR_NAME
#endif

#include "toteload.h"

typedef struct VECTOR_NAME {
  usize len;
  usize cap;
  VECTOR_TYPE *data;
} VECTOR_NAME;

b32  Cat(VECTOR_NAME, _is_empty)(VECTOR_NAME *v);
void Cat(VECTOR_NAME, _push)(VECTOR_NAME *v, VECTOR_TYPE x);

#ifdef VECTOR_IMPLEMENTATION
#endif
