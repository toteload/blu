#include "blu.hh"

enum CastResult {
  CastResult_ok,
  CastResult_integer_value_out_of_range,
};

struct EvalCastContext {
  ValueStore *values;
  TypeInterner *types;
};

i64 read_signed_integer_extend(Type* type, void *payload) {
  Assert(type->kind == Type_integer);
  Assert(type->integer.signedness == Signed);

  i64 res;

  // clang-format off
  switch (type->integer.bitwidth) {
  case  8: { i8  x; memcpy(&x, payload, 1); res = x; } break;
  case 16: { i16 x; memcpy(&x, payload, 2); res = x; } break;
  case 32: { i32 x; memcpy(&x, payload, 4); res = x; } break;
  case 64: { i64 x; memcpy(&x, payload, 8); res = x; } break;
  default: Unreachable();
  }
  // clang-format on

  return res;
}

u64 read_unsigned_integer_extend(Type* type, void *payload) {
  Assert(type->kind == Type_integer);
  Assert(type->integer.signedness == Unsigned);

  u64 res = 0;
  memcpy(&res, payload, type->integer.bitwidth / 8);

  return res;
}

// You may assume that the cast is valid and has been typechecked.
CastResult eval_cast(
  EvalCastContext *context,
  TypeIndex type_idx_src, void *payload_src,
  TypeIndex type_idx_dst, void *payload_dst
) {
  if (type_idx_dst == type_idx_src) {
    auto size_info = types->size_info(type_idx_dst);
    memcpy(data, val->data, size_info.size);
    return true;
  }

  Type *type_dst = context->types->get(type_idx_dst);
  Type *type_src = context->types->get(type_idx_src);

  if (type_src->kind == Type_literal_function && type_dst->kind == Type_function) {
    auto size_info = types->size_info(type_idx_dst);
    memcpy(data, val->data, size_info.size);
    return true;
  }

  if (type_src->kind == Type_integer && type_dst->kind == Type_integer) {
    if (type_dst->integer.signedness == Signed && type_src->integer.signedness == Signed) {
      i64 i = read_signed_integer_extend(type_src, payload_src);

      i64 lo = int_value_min(type_dst->integer.bitwidth);
      i64 hi = int_value_max(type_dst->integer.bitwidth);

      if (i < lo || i > hi) {
        return CastResult_integer_value_out_of_range;
      }
    } else if (type_dst->integer.signedness == Unsigned && type_src->integer.signedness == Signed) {
      i64 i = read_value_i64(val_idx);

      if (i < 0) {
        return CastResult_integer_value_out_of_range;
      }

      if (type_src->integer.bitwidth > type_dst->integer.bitwidth) {
        u64 u = cast<u64>(i);

        u64 hi = uint_value_max(type_dst->integer.bitwidth);

        if (u > hi) {
          return CastResult_integer_value_out_of_range;
        }
      }
    } else if (type_dst->integer.signedness == Signed && type_src->integer.signedness == Unsigned) {
      u64 i = read_unsigned_integer_extend(val_idx);

      u64 hi = cast<u64>(int_value_max(type_dst->integer.bitwidth));

      if (i > hi) {
        return CastResult_integer_value_out_of_range;
      }
    } else {
      u64 i = read_value_u64(val_idx);

      u64 hi = uint_value_max(type_dst->integer.bitwidth);

      if (i > hi) {
        return CastResult_integer_value_out_of_range;
      }
    }

    auto size_info = types->size_info(type_idx_dst);

    memcpy(payload_dst, payload_src, size_info.size);

    return true;
  }

  if (type_src->kind == Type_array && type_dst->kind == Type_slice) {
    u32 count = type_src->array.size;

    Value *v;
    *result = values->alloc_value(&v);

    auto size_info = types->size_info(type_idx_dst);
    auto data      = values->alloc_data(size_info);

    *cast<ValueSlice *>(data) = {
      .len   = count,
      .items = val->data,
    };

    *v = {
      .type = type_idx_dst,
      .data = data,
    };

    return true;
  }

  if (type_src->kind == Type_sequence) {
    TypeIndex base_type;
    if (type_dst->kind == Type_slice) {
      base_type = type_dst->slice.base_type;
    } else if (type_dst->kind == Type_array) {
      base_type = type_dst->array.base_type;
    } else {
      Unreachable();
    }

    u32  count     = type_src->sequence.count;
    auto size_info = types->size_info(base_type);

    ValueIndex *sequence_items = cast<ValueIndex *>(val->data);

    Todo();

    // auto items = values->alloc_data(size_info, count);
    // for (u32 i = 0; i < count; i++) {
    //   Try(coerce_value(base_type, sequence_items[i], ptr_offset(items, size_info.stride * i)));
    // }

    // if (type_dst->kind == Type_array) {
    //   *cast<void **>(out) = items;
    // } else {
    //   *cast<ValueSlice *>(out) = {.len = count, .items = items};
    // }

    return true;
  }

  Todo();

  return true;
}

i32 main() {
  Arena arena{};
  arena.init(KiB(16));

  // integer -> integer
  
  u16 bitwidths[] = {8,16,32,64};

  // Casting of an integer to equally wide or wider integer type.
  for (u32 i = 0; i < 4; i++) {
    TypeIndex type_from = types.add();

    for (u32 j = i; j < 4; j++) {
      TypeIndex type_to = types.add();


    }
  }

  // array -> slice

  

  return 0;
}
