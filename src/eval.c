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

internal b32 is_type_complete(TypeInterner *types, TypeIndex idx) {
  Type *t = types_get(types, idx);

  switch (Cast(TypeKind, t->kind)) {
  case Type_integer:
  case Type_bool:
  case Type_comptime_int:
  case Type_never:
  case Type_nil:
  case Type_type:
    return True;
  case Type_array: return t->data.array.base_type != 0;
  case Type_slice: return t->data.slice.base_type != 0;
  case Type_pointer: return t->data.pointer.base_type != 0;
  case Type_function: {
    if (t->data.function.return_type == 0) {
      return False;
    }

    for (u32 i = 0; i < t->data.function.param_count; i++) {
      if (t->data.function.param_types[i] == 0) {
        return False;
      }
    }
  } break;
  }

  return True;
}

u32 eval_unify(Arena *scratch, TypeInterner *types, TypeIndex a, TypeIndex b, TypeIndex *unified) {
  if (a == 0 && b == 0) {
    return UnifyResult_type_is_incomplete;
  }

  if (a == b) {
    *unified = a;
    return UnifyResult_ok;
  }

  if (a == 0) {
    if (!is_type_complete(types, b)) {
      return UnifyResult_type_is_incomplete;
    }

    *unified = b;
    return UnifyResult_ok;
  }

  if (b == 0) {
    if (!is_type_complete(types, a)) {
      return UnifyResult_type_is_incomplete;
    }

    *unified = a;
    return UnifyResult_ok;
  }

  Type *type_lhs = types_get(types, a);
  Type *type_rhs = types_get(types, b);

  if (type_lhs->kind == Type_function && type_rhs->kind == Type_function) {
    u32 param_count = type_lhs->data.function.param_count;
    if (param_count != type_rhs->data.function.param_count) {
      return UnifyResult_types_cannot_be_unified;
    }

    TypeIndex return_type;
    u32 err = eval_unify(scratch, types, type_lhs->data.function.return_type, type_rhs->data.function.return_type, &return_type);
    if (err) {
      return UnifyResult_types_cannot_be_unified;
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

    TypeIndex res = types_add(types, f);

    if (!err) {
      *unified = res;
    }

    arena_scope_end(scratch, snapshot);

    if (!is_type_complete(types, res)) {
      return UnifyResult_type_is_incomplete;
    }

    return UnifyResult_ok;
  }

  if ((type_lhs->kind == Type_comptime_int && type_rhs->kind == Type_integer) || (type_lhs->kind == Type_integer && type_rhs->kind == Type_comptime_int)) {
    *unified = b;
    return UnifyResult_ok;
  }

  if (type_lhs->kind == Type_array && type_rhs->kind == Type_array) {
    if (type_lhs->data.array.size != type_rhs->data.array.size) {
      return UnifyResult_types_cannot_be_unified;
    }

    TypeIndex base_type;
    u32 err = eval_unify(scratch, types, type_lhs->data.array.base_type, type_rhs->data.array.base_type, &base_type);
    if (err) {
      return err;
    }

    TypeIndex res = types_add(types, &(Type){
      .kind = Type_array,
      .data.array = { .base_type = base_type, .size = type_lhs->data.array.size },
    });

    *unified = res;

    return UnifyResult_ok;
  }

  if (type_lhs->kind == Type_slice && type_rhs->kind == Type_slice) {
    TypeIndex base_type;
    u32 err = eval_unify(scratch, types, type_lhs->data.slice.base_type, type_rhs->data.slice.base_type, &base_type);
    if (err) {
      return err;
    }

    TypeIndex res = types_add(types, &(Type){
      .kind = Type_slice,
      .data.slice = { .base_type = base_type },
    });

    *unified = res;

    return UnifyResult_ok;
  }

  if (type_lhs->kind == Type_pointer && type_rhs->kind == Type_pointer) {
    TypeIndex base_type;
    u32 err = eval_unify(scratch, types, type_lhs->data.pointer.base_type, type_rhs->data.pointer.base_type, &base_type);
    if (err) {
      return err;
    }

    TypeIndex res = types_add_pointer(types, base_type);

    *unified = res;

    return UnifyResult_ok;
  }

  return UnifyResult_types_cannot_be_unified;
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

  if (type_val->kind == Type_function && type_dst->kind == Type_function) {
    u32 param_count = type_val->data.function.param_count;
    if (param_count != type_dst->data.function.param_count) {
      return CoerceResult_invalid_coercion_types;
    }

    TypeIndex return_type = type_val->data.function.return_type;

    if (return_type && return_type != type_dst->data.function.return_type) {
      return CoerceResult_invalid_coercion_types;
    }

    for (u32 i = 0; i < param_count; i++) {
      TypeIndex param_type = type_val->data.function.param_types[i];
      if (param_type && param_type != type_dst->data.function.param_types[i]) {
        return CoerceResult_invalid_coercion_types;
      }
    }

    Value *p;
    ValueIndex idx = values_alloc(values, &p);

    u32 size = sizeof(ValueFunc);
    void *data = values_alloc_data(values, size, Align_of(ValueFunc));
    memcpy(data, val->data, size);
    *p = (Value){
      .type = dst,
      .data = data,
      .data_size = size,
    };

    *res = idx;
    return CoerceResult_ok;
  }

  return CoerceResult_invalid_coercion_types;
}

b32 eval_int_add_safe(TypeInteger int_type, void *lhs, void *rhs, void *res) {
  Assert(int_type.bitwidth % 8 == 0);

  b32 overflow;

  if (int_type.signedness == Signed) {
    // clang-format off
    switch (int_type.bitwidth) {
    case 8:  { overflow = __builtin_add_overflow(*Cast(i8*,lhs),  *Cast(i8*,rhs),  Cast(i8*,res));  } break;
    case 16: { overflow = __builtin_add_overflow(*Cast(i16*,lhs), *Cast(i16*,rhs), Cast(i16*,res)); } break;
    case 32: { overflow = __builtin_add_overflow(*Cast(i32*,lhs), *Cast(i32*,rhs), Cast(i32*,res)); } break;
    case 64: { overflow = __builtin_add_overflow(*Cast(i64*,lhs), *Cast(i64*,rhs), Cast(i64*,res)); } break;
    default: Unreachable();
    }
    // clang-format on
  } else {
    // clang-format off
    switch (int_type.bitwidth) {
    case 8:  { overflow = __builtin_add_overflow(*Cast(u8*,lhs),  *Cast(u8*,rhs),  Cast(u8*,res));  } break;
    case 16: { overflow = __builtin_add_overflow(*Cast(u16*,lhs), *Cast(u16*,rhs), Cast(u16*,res)); } break;
    case 32: { overflow = __builtin_add_overflow(*Cast(u32*,lhs), *Cast(u32*,rhs), Cast(u32*,res)); } break;
    case 64: { overflow = __builtin_add_overflow(*Cast(u64*,lhs), *Cast(u64*,rhs), Cast(u64*,res)); } break;
    default: Unreachable();
    }
    // clang-format on
  }

  return !overflow;
}

b32 eval_int_sub_safe(TypeInteger int_type, void *lhs, void *rhs, void *res) {
  Assert(int_type.bitwidth % 8 == 0);

  b32 overflow;

  if (int_type.signedness == Signed) {
    // clang-format off
    switch (int_type.bitwidth) {
    case 8:  { overflow = __builtin_sub_overflow(*Cast(i8*,lhs),  *Cast(i8*,rhs),  Cast(i8*,res));  } break;
    case 16: { overflow = __builtin_sub_overflow(*Cast(i16*,lhs), *Cast(i16*,rhs), Cast(i16*,res)); } break;
    case 32: { overflow = __builtin_sub_overflow(*Cast(i32*,lhs), *Cast(i32*,rhs), Cast(i32*,res)); } break;
    case 64: { overflow = __builtin_sub_overflow(*Cast(i64*,lhs), *Cast(i64*,rhs), Cast(i64*,res)); } break;
    default: Unreachable();
    }
    // clang-format on
  } else {
    // clang-format off
    switch (int_type.bitwidth) {
    case 8:  { overflow = __builtin_sub_overflow(*Cast(u8*,lhs),  *Cast(u8*,rhs),  Cast(u8*,res));  } break;
    case 16: { overflow = __builtin_sub_overflow(*Cast(u16*,lhs), *Cast(u16*,rhs), Cast(u16*,res)); } break;
    case 32: { overflow = __builtin_sub_overflow(*Cast(u32*,lhs), *Cast(u32*,rhs), Cast(u32*,res)); } break;
    case 64: { overflow = __builtin_sub_overflow(*Cast(u64*,lhs), *Cast(u64*,rhs), Cast(u64*,res)); } break;
    default: Unreachable();
    }
    // clang-format on
  }

  return !overflow;
}

b32 eval_int_mul_safe(TypeInteger int_type, void *lhs, void *rhs, void *res) {
  Assert(int_type.bitwidth % 8 == 0);

  b32 overflow;

  if (int_type.signedness == Signed) {
    // clang-format off
    switch (int_type.bitwidth) {
    case 8:  { overflow = __builtin_mul_overflow(*Cast(i8*,lhs),  *Cast(i8*,rhs),  Cast(i8*,res));  } break;
    case 16: { overflow = __builtin_mul_overflow(*Cast(i16*,lhs), *Cast(i16*,rhs), Cast(i16*,res)); } break;
    case 32: { overflow = __builtin_mul_overflow(*Cast(i32*,lhs), *Cast(i32*,rhs), Cast(i32*,res)); } break;
    case 64: { overflow = __builtin_mul_overflow(*Cast(i64*,lhs), *Cast(i64*,rhs), Cast(i64*,res)); } break;
    default: Unreachable();
    }
    // clang-format on
  } else {
    // clang-format off
    switch (int_type.bitwidth) {
    case 8:  { overflow = __builtin_mul_overflow(*Cast(u8*,lhs),  *Cast(u8*,rhs),  Cast(u8*,res));  } break;
    case 16: { overflow = __builtin_mul_overflow(*Cast(u16*,lhs), *Cast(u16*,rhs), Cast(u16*,res)); } break;
    case 32: { overflow = __builtin_mul_overflow(*Cast(u32*,lhs), *Cast(u32*,rhs), Cast(u32*,res)); } break;
    case 64: { overflow = __builtin_mul_overflow(*Cast(u64*,lhs), *Cast(u64*,rhs), Cast(u64*,res)); } break;
    default: Unreachable();
    }
    // clang-format on
  }

  return !overflow;
}

u32 eval_int_div_safe(TypeInteger int_type, void *lhs, void *rhs, void *res) {
  Assert(int_type.bitwidth % 8 == 0);

  if (int_type.signedness == Signed) {
    // clang-format off
    switch (int_type.bitwidth) {
    case 8:  { if (*Cast(i8*,rhs)  == 0) return IntDivSafe_zero_division; if (*Cast(i8*,rhs)  == -1 && *Cast(i8*,lhs)  == INT8_MIN)  return IntDivSafe_overflow; *Cast(i8*,res)  = *Cast(i8*,lhs)  / *Cast(i8*,rhs);  } break;
    case 16: { if (*Cast(i16*,rhs) == 0) return IntDivSafe_zero_division; if (*Cast(i16*,rhs) == -1 && *Cast(i16*,lhs) == INT16_MIN) return IntDivSafe_overflow; *Cast(i16*,res) = *Cast(i16*,lhs) / *Cast(i16*,rhs); } break;
    case 32: { if (*Cast(i32*,rhs) == 0) return IntDivSafe_zero_division; if (*Cast(i32*,rhs) == -1 && *Cast(i32*,lhs) == INT32_MIN) return IntDivSafe_overflow; *Cast(i32*,res) = *Cast(i32*,lhs) / *Cast(i32*,rhs); } break;
    case 64: { if (*Cast(i64*,rhs) == 0) return IntDivSafe_zero_division; if (*Cast(i64*,rhs) == -1 && *Cast(i64*,lhs) == INT64_MIN) return IntDivSafe_overflow; *Cast(i64*,res) = *Cast(i64*,lhs) / *Cast(i64*,rhs); } break;
    default: Unreachable();
    }
    // clang-format on
  } else {
    // clang-format off
    switch (int_type.bitwidth) {
    case 8:  { if (*Cast(u8*,rhs)  == 0) return IntDivSafe_zero_division; *Cast(u8*,res)  = *Cast(u8*,lhs)  / *Cast(u8*,rhs);  } break;
    case 16: { if (*Cast(u16*,rhs) == 0) return IntDivSafe_zero_division; *Cast(u16*,res) = *Cast(u16*,lhs) / *Cast(u16*,rhs); } break;
    case 32: { if (*Cast(u32*,rhs) == 0) return IntDivSafe_zero_division; *Cast(u32*,res) = *Cast(u32*,lhs) / *Cast(u32*,rhs); } break;
    case 64: { if (*Cast(u64*,rhs) == 0) return IntDivSafe_zero_division; *Cast(u64*,res) = *Cast(u64*,lhs) / *Cast(u64*,rhs); } break;
    default: Unreachable();
    }
    // clang-format on
  }

  return IntDivSafe_ok;
}
