#ifndef REF_NAME
#error "'REF_NAME' must be defined"
#endif

#ifndef REF_FUNCTION_PREFIX
#error "'REF_FUNCTION_PREFIX' must be defined"
#endif

#include "toteload.h"

typedef struct { u32 x; } REF_NAME;

always_inline b32 Cat(REF_FUNCTION_PREFIX, _is_value)(REF_NAME r) {
  return (r.x & REF_TAG_MASK) == REF_TAG_VALUE;
}

always_inline b32 Cat(REF_FUNCTION_PREFIX, _is_some_value)(REF_NAME r) {
  return r.x != 0 && Cat(REF_FUNCTION_PREFIX, _is_value)(r);
}

always_inline b32 Cat(REF_FUNCTION_PREFIX, _is_instruction)(REF_NAME r) {
  return (r.x & REF_TAG_MASK) == REF_TAG_INSTRUCTION;
}

always_inline b32 Cat(REF_FUNCTION_PREFIX, _is_offset)(REF_NAME r) {
  return (r.x & REF_TAG_MASK) == REF_TAG_OFFSET;
}

always_inline b32 Cat(REF_FUNCTION_PREFIX, _is_nil)(REF_NAME r) {
  return r.x == 0;
}

always_inline ValueKey Cat(REF_FUNCTION_PREFIX, _to_value)(REF_NAME r) {
  ASSERT_SAFE(Cat(REF_FUNCTION_PREFIX, _is_value)(r));
  return (ValueKey){r.x};
}

always_inline InstructionOffset Cat(REF_FUNCTION_PREFIX, _to_instruction)(REF_NAME r) {
  ASSERT_SAFE(Cat(REF_FUNCTION_PREFIX, _is_instruction)(r));
  return r.x & ~REF_TAG_MASK;
}

always_inline ValueOffset Cat(REF_FUNCTION_PREFIX, _to_offset)(REF_NAME r) {
  ASSERT_SAFE(Cat(REF_FUNCTION_PREFIX, _is_offset)(r));
  return r.x & ~REF_TAG_MASK;
}

always_inline REF_NAME Cat(REF_FUNCTION_PREFIX, _from_offset)(ValueOffset offset) {
  ASSERT_SAFE((offset & REF_TAG_MASK) == 0);
  return (REF_NAME){offset | REF_TAG_OFFSET};
}

always_inline REF_NAME Cat(REF_FUNCTION_PREFIX, _from_instruction)(InstructionOffset idx) {
  ASSERT_SAFE((idx & REF_TAG_MASK) == 0);
  return (REF_NAME){idx | REF_TAG_INSTRUCTION};
}

always_inline REF_NAME Cat(REF_FUNCTION_PREFIX, _from_value)(ValueKey idx) {
  ASSERT_SAFE((idx.x & REF_TAG_MASK) == 0);
  return (REF_NAME){idx.x};
}

always_inline u32 Cat(REF_FUNCTION_PREFIX, _to_u32)(REF_NAME r) {
  return r.x;
}

#undef REF_NAME
#undef REF_FUNCTION_PREFIX
