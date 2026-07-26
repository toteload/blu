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
  UnifyResult_unable_to_unify,
  UnifyResult_no_concrete_type_provided,
} UnifyResult;

u32 eval_unify(Arena *scratch, TypeInterner *types, TypeIndex a, TypeIndex b, TypeIndex *unified);

enum CoerceResult {
  CoerceResult_ok,
  CoerceResult_invalid_coercion_types,
  CoerceResult_comptime_int_value_out_of_range,
};

u32 eval_coerce(TypeInterner *types, ValueStore *values, TypeIndex dst, Value *val, ValueIndex *res);

#endif // EVAL_H
