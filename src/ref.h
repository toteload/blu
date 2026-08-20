#ifndef REF_NAME
#error "'REF_NAME' must be defined"
#endif

#ifndef REF_FUNCTION_PREFIX
#error "'REF_FUNCTION_PREFIX' must be defined"
#endif

#include "toteload.h"

#define BITMASK_REF_IS_INSTRUCTION_INDEX (Cast(u32, 1) << 31)

typedef struct { u32 x; } REF_NAME;

always_inline b32 Cat(REF_FUNCTION_PREFIX, _is_value)(REF_NAME r) {
  return (r.x & BITMASK_REF_IS_INSTRUCTION_INDEX) == 0;
}

always_inline b32 Cat(REF_FUNCTION_PREFIX, _is_some_value)(REF_NAME r) {
  return r.x != 0 && Cat(REF_FUNCTION_PREFIX, _is_value)(r);
}

always_inline b32 Cat(REF_FUNCTION_PREFIX, _is_instruction)(REF_NAME r) {
  return (r.x & BITMASK_REF_IS_INSTRUCTION_INDEX) != 0;
}

always_inline b32 Cat(REF_FUNCTION_PREFIX, _is_nil)(REF_NAME r) {
  return r.x == 0;
}

always_inline ValueIndex Cat(REF_FUNCTION_PREFIX, _to_value)(REF_NAME r) {
  return r.x;
}

always_inline ValueIndex Cat(REF_FUNCTION_PREFIX, _to_instruction)(REF_NAME r) {
  return r.x & ~BITMASK_REF_IS_INSTRUCTION_INDEX;
}

always_inline REF_NAME Cat(REF_FUNCTION_PREFIX, _from_instruction)(InstructionIndex idx) {
  return (REF_NAME){idx | BITMASK_REF_IS_INSTRUCTION_INDEX};
}

always_inline REF_NAME Cat(REF_FUNCTION_PREFIX, _from_value)(ValueIndex idx) {
  return (REF_NAME){idx};
}

always_inline u32 Cat(REF_FUNCTION_PREFIX, _to_u32)(REF_NAME r) {
  return r.x;
}

#undef REF_NAME
#undef REF_FUNCTION_PREFIX
