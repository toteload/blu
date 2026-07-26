#include "types.h"
#include "value.h"
#include "eval.h"

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

u32 eval_unify(Arena *scratch, TypeInterner *types, TypeIndex a, TypeIndex b, TypeIndex *unified) {
  if (a == b) {
    *unified = a;
    return UnifyResult_ok;
  }

  if (a == 0 && b == 0) {
    return UnifyResult_no_concrete_type_provided;
  }

  if (a == 0) {
    *unified = b;
    return UnifyResult_ok;
  }

  if (b == 0) {
    *unified = a;
    return UnifyResult_ok;
  }

  Type *type_lhs = types_get(types, a);
  Type *type_rhs = types_get(types, b);

  if (type_lhs->kind == Type_function && type_rhs->kind == Type_function) {
    u32 param_count = type_lhs->data.function.param_count;
    if (param_count != type_rhs->data.function.param_count) {
      return UnifyResult_unable_to_unify;
    }

    TypeIndex return_type;
    u32 err = eval_unify(scratch, types, type_lhs->data.function.return_type, type_rhs->data.function.return_type, &return_type);
    if (err) {
      return UnifyResult_unable_to_unify;
    }

    ArenaSnapshot snapshot = arena_scope_begin(scratch);

    Type *f = arena_push_type_function(scratch, param_count);
    f->kind = Type_function;
    f->data.function.return_type = return_type;

    for (u32 i = 0; i < param_count; i++) {
      TypeIndex param_type;
      err = eval_unify(scratch, types, type_lhs->data.function.param_types[i], type_rhs->data.function.param_types[i], &param_type);
      if (err) {
        break;
      }

      f->data.function.param_types[i] = param_type;
    }

    if (!err) {
      *unified = types_add(types, f);
    }

    arena_scope_end(scratch, snapshot);

    return UnifyResult_ok;
  }

  if ((type_lhs->kind == Type_comptime_int && type_rhs->kind == Type_integer) || (type_lhs->kind == Type_integer && type_rhs->kind == Type_comptime_int)) {
    *unified = b;
    return UnifyResult_ok;
  }

  Todo();

  return UnifyResult_unable_to_unify;
}

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
    TypeInteger comptime_int = {
      .signedness = Signed,
      .bitwidth   = sizeof(ComptimeInt) * 8,
    };

    TypeSizeInfo size_info = types_size_info_by_index(types, dst);
    void *data = values_alloc_data(values, size_info.size, size_info.align);

    u32 err = eval_cast_int(comptime_int, val->data, type_dst->data.integer, data);
    if (err) {
      values_dealloc_data(values, data, size_info.size);
      return CoerceResult_comptime_int_value_out_of_range;
    }

    Value *v;
    ValueIndex idx = values_alloc(values, &v);
    *v = (Value){
      .type = dst,
      .data = data,
      .data_size = size_info.size,
    };

    *res = idx;

    return CoerceResult_ok;
  }

  return CoerceResult_invalid_coercion_types;
}
