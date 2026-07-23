#include "types.h"
#include "value.h"

enum CastResult {
  CastResult_ok,
  CastResult_integer_value_out_of_range,
};

internal i64 read_signed_integer_extend(u16 bitwidth, void *payload) {
  i64 res;

  // clang-format off
  switch (bitwidth) {
  case  8: { i8  x; memcpy(&x, payload, 1); res = x; } break;
  case 16: { i16 x; memcpy(&x, payload, 2); res = x; } break;
  case 32: { i32 x; memcpy(&x, payload, 4); res = x; } break;
  case 64: { i64 x; memcpy(&x, payload, 8); res = x; } break;
  default: Unreachable();
  }
  // clang-format on

  return res;
}

internal u64 read_unsigned_integer_extend(u16 bitwidth, void *payload) {
  u64 res = 0;
  memcpy(&res, payload, bitwidth / 8);

  return res;
}

internal i64 int_value_min(u16 bitwidth) {
  // clang-format off
  switch (bitwidth) {
  case 8:  return INT8_MIN;
  case 16: return INT16_MIN;
  case 32: return INT32_MIN;
  case 64: return INT64_MIN;
  default: Unreachable();
  }
  // clang-format on
}

internal i64 int_value_max(u16 bitwidth) {
  // clang-format off
  switch (bitwidth) {
  case 8:  return INT8_MAX;
  case 16: return INT16_MAX;
  case 32: return INT32_MAX;
  case 64: return INT64_MAX;
  default: Unreachable();
  }
  // clang-format on
}

internal u64 uint_value_max(u16 bitwidth) {
  // clang-format off
  switch (bitwidth) {
  case 8:  return UINT8_MAX;
  case 16: return UINT16_MAX;
  case 32: return UINT32_MAX;
  case 64: return UINT64_MAX;
  default: Unreachable();
  }
  // clang-format on
}

// ASSUME: the cast of `type_idx_src` to `type_idx_dst` is valid.
// ASSUME: the bitwidth of the integers is a multiple of 8.
u32 eval_cast_int(
  TypeInteger type_src, void *payload_src,
  TypeInteger type_dst, void *payload_dst
) {
  if (type_src.signedness == type_dst.signedness && type_src.bitwidth == type_dst.bitwidth) {
    memcpy(payload_dst, payload_src, type_dst.bitwidth / 8);
    return CastResult_ok;
  }

  if (type_dst.signedness == Signed && type_src.signedness == Signed) {
    i64 i = read_signed_integer_extend(type_src.bitwidth, payload_src);

    i64 lo = int_value_min(type_dst.bitwidth);
    i64 hi = int_value_max(type_dst.bitwidth);

    if (i < lo || i > hi) {
      return CastResult_integer_value_out_of_range;
    }

    memcpy(payload_dst, &i, type_dst.bitwidth / 8);

    return CastResult_ok;
  }

  if (type_dst.signedness == Unsigned && type_src.signedness == Signed) {
    i64 i = read_signed_integer_extend(type_src.bitwidth, payload_src);

    if (i < 0) {
      return CastResult_integer_value_out_of_range;
    }

    if (type_src.bitwidth > type_dst.bitwidth) {
      u64 u = Cast(u64, i);

      u64 hi = uint_value_max(type_dst.bitwidth);

      if (u > hi) {
        return CastResult_integer_value_out_of_range;
      }
    }

    memcpy(payload_dst, &i, type_dst.bitwidth / 8);

    return CastResult_ok;
  }

  if (type_dst.signedness == Signed && type_src.signedness == Unsigned) {
    u64 i = read_unsigned_integer_extend(type_src.bitwidth, payload_src);

    u64 hi = Cast(u64, int_value_max(type_dst.bitwidth));

    if (i > hi) {
      return CastResult_integer_value_out_of_range;
    }

    memcpy(payload_dst, &i, type_dst.bitwidth / 8);

    return CastResult_ok;
  }

  if (type_dst.signedness == Unsigned && type_src.signedness == Unsigned) {
    u64 i = read_unsigned_integer_extend(type_src.bitwidth, payload_src);

    u64 hi = uint_value_max(type_dst.bitwidth);

    if (i > hi) {
      return CastResult_integer_value_out_of_range;
    }

    memcpy(payload_dst, &i, type_dst.bitwidth / 8);

    return CastResult_ok;
  }

  Unreachable();

  return CastResult_ok;
}

u32 eval_unify(TypeIndex a, TypeIndex b, TypeIndex *unified) {
  Todo();
}

enum CoerceResult {
  CoerceResult_ok,
  CoerceResult_invalid_coercion_types,
  CoerceResult_comptime_int_value_too_big,
};

u32 eval_coerce(TypeInterner *types, ValueStore *values, TypeIndex dst, Value *val, ValueIndex *res) {
  if (dst == val->type) {
    Value *p;
    ValueIndex idx = values_alloc(values, &p);

    TypeSizeInfo size_info = types_size_info_by_index(types, dst);
    void *data = values_alloc_data(values, size_info.size, size_info.align);
    memcpy(data, val->data, size_info.size);

    *p = (Value){
      .type = dst,
      .data = data,
      .data_size = size_info.size,
    };

    *res = idx;
    return CoerceResult_ok;
  }

  Type *type_dst = types_get(types, dst);
  Type *type_val = types_get(types, val->type);

  if (type_val->kind == Type_comptime_int && type_dst->kind == Type_integer) {
    Panic();
    // This may still fail! The value of the comptime_int may be too big.
    return CoerceResult_ok;
  }

  return CoerceResult_invalid_coercion_types;
}
