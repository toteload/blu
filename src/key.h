#ifndef KEY_NAME
#error
#endif

#ifndef KEY_FUNCTION_PREFIX
#error
#endif

#include "toteload.h"

typedef struct { u32 x; } KEY_NAME;

always_inline b8 Cat(KEY_FUNCTION_PREFIX, _is_nil)(KEY_NAME key) {
  return key.x == 0;
}

always_inline b8 Cat(KEY_FUNCTION_PREFIX, _is_some)(KEY_NAME key) {
  return key.x != 0;
}

always_inline u32 Cat(KEY_FUNCTION_PREFIX, _get)(KEY_NAME key) {
  return key.x;
}

always_inline b8 Cat(KEY_FUNCTION_PREFIX, _eq)(KEY_NAME a, KEY_NAME b) {
  return a.x == b.x;
}

#undef KEY_NAME
#undef KEY_FUNCTION_PREFIX
