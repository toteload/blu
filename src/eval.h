#ifndef EVAL_H
#define EVAL_H

#include "blu.h"

typedef enum {
  CastResult_ok,
  CastResult_integer_value_out_of_range,
} CastResult;

// ASSUME: the cast of `type_idx_src` to `type_idx_dst` is valid.
// ASSUME: the bitwidth of the integers is a multiple of 8.
u32 eval_cast_int(TypeInteger type_src, void *payload_src, TypeInteger type_dst, void *payload_dst);

typedef enum {
  UnifyResult_ok,
  UnifyResult_types_cannot_be_unified,

  // Types could be unified, but resulted in an incomplete type.
  // When this is returned, `unified` is written to and the incomplete type can be retrieved with it.
  UnifyResult_type_is_incomplete,
} UnifyResult;

u32 eval_unify(Arena *scratch, TypeInterner *types, TypeIndex a, TypeIndex b, TypeIndex *unified);

enum CoerceResult {
  CoerceResult_ok,
  CoerceResult_invalid_coercion_types,
  CoerceResult_comptime_int_value_out_of_range,
};

u32 eval_coerce(TypeInterner *types, ValueStore *values, TypeIndex dst, Value *val, ValueIndex *res);

// Returns False if the addition overflowed.
b32 eval_int_add(TypeInteger int_type, void *lhs, void *rhs, void *res);
// Returns False if the subtraction overflowed.
b32 eval_int_sub(TypeInteger int_type, void *lhs, void *rhs, void *res);
// Returns False if the multiplication overflowed.
b32 eval_int_mul(TypeInteger int_type, void *lhs, void *rhs, void *res);
// Returns False if rhs is zero, or if lhs / rhs overflows (INT_MIN / -1).
b32 eval_int_div(TypeInteger int_type, void *lhs, void *rhs, void *res);

#endif // EVAL_H
