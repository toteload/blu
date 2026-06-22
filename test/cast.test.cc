#include "types.h"

enum CastResult {
  CastResult_ok,
  CastResult_integer_value_out_of_range,
};

i64 read_signed_integer_extend(u16 bitwidth, void *payload) {
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

u64 read_unsigned_integer_extend(u16 bitwidth, void *payload) {
  u64 res = 0;
  memcpy(&res, payload, bitwidth / 8);

  return res;
}

// It is assumed that the cast of `type_idx_src` to `type_idx_dst` is valid.
u32 eval_cast_int(
  TypeInteger type_src, void *payload_src,
  TypeInteger type_dst, void *payload_dst
) {
  Assert(type_src.bitwidth % 8 == 0);
  Assert(type_dst.bitwidth % 8 == 0);

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

i32 main() {
  Arena arena{};
  arena.init(KiB(16));

  TypeInterner types;

  // integer -> integer
  
  u16 bitwidths[] = {8,16,32,64};

  // Casting of an integer to equally wide or wider integer type of identical signedness.
  // This cast should always succeed for any value of the source integer.
  for (u8 si = 0; si < 2; si++) {
    u8 signedness = si; 

    for (u32 i = 0; i < 4; i++) {
      TypeIndex type_src = types_add(&types, &(Type){
        .kind = Type_integer,
        .data.integer = {
          .signedness = signedness,
          .bitwidth   = bitwidths[i],
        },
      });

      void *payload_src = arena_push_array(u8, &arena, bitwidths[i] / 8);

      for (u32 j = i; j < 4; j++) {
        TypeIndex type_dst = types_add(&types, &(Type){
          .kind = Type_integer,
          .data.integer = {
            .signedness = signedness,
            .bitwidth   = bitwidths[j],
          },
        });

        void *payload_dst = arena_push_array(u8, &arena, bitwidths[j] / 8);

        Todo();

        // TODO loop over values for which to try this cast

        u32 err = eval_cast(
          &types,
          type_src, payload_src,
          type_dst, payload_dst,
        );
      }
    }
  }

  // Casting of an integer to narrower integer type of identical signedness.
  // This cast can fail if the value is outside the value range of the target integer type.
  // TODO

  // Casting of unsigned integer to wider signed integer type.
  // This cast should always succeed for any value.
  // TODO

  // Casting of unsigned integer to narrower or equal width signed integer.
  // This cast can fail.
  // TODO

  // Casting of signed integer to unsigned integer.
  // TODO

  // array -> slice

  return 0;
}
