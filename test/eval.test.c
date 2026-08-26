#include "toteload.h"
#include "blu.h"
#include "types.h"
#include "value.h"
#include "eval.h"
#include "test.h"

internal void *test_alloc_fn(void *ctx, void *p, usize old_byte_size, usize new_byte_size, u32 align) {
  Unused(ctx, old_byte_size, align);

  if (!is_null(p) && new_byte_size == 0) {
    free(p);
    return Null;
  }

  return realloc(p, new_byte_size);
}

internal TypeInterner test_types_init(Arena *arena, Arena *scratch) {
  TypeInterner types;
  types_init(
    &types,
    &(InternerOptions){
      .arena          = arena,
      .map_allocator  = (Allocator){ .fn = test_alloc_fn },
      .map_initial_size = 32,
      .context        = scratch,
    }
  );
  return types;
}

internal TypeInteger int_type(u8 signedness, u16 bitwidth) {
  return (TypeInteger){ .signedness = signedness, .bitwidth = bitwidth };
}

typedef union {
  i8  as_i8;  u8  as_u8;
  i16 as_i16; u16 as_u16;
  i32 as_i32; u32 as_u32;
  i64 as_i64; u64 as_u64;
} IntBuf;

internal void write_int(TypeInteger type, i64 value, IntBuf *out) {
  switch (type.bitwidth) {
    case 8:  if (type.signedness == Signed) out->as_i8  = Cast(i8,value);  else out->as_u8  = Cast(u8,value);  break;
    case 16: if (type.signedness == Signed) out->as_i16 = Cast(i16,value); else out->as_u16 = Cast(u16,value); break;
    case 32: if (type.signedness == Signed) out->as_i32 = Cast(i32,value); else out->as_u32 = Cast(u32,value); break;
    case 64: if (type.signedness == Signed) out->as_i64 = value;           else out->as_u64 = Cast(u64,value); break;
  }
}

internal i64 read_int(TypeInteger type, IntBuf *in) {
  switch (type.bitwidth) {
    case 8:  return type.signedness == Signed ? Cast(i64,in->as_i8)  : Cast(i64,in->as_u8);
    case 16: return type.signedness == Signed ? Cast(i64,in->as_i16) : Cast(i64,in->as_u16);
    case 32: return type.signedness == Signed ? Cast(i64,in->as_i32) : Cast(i64,in->as_u32);
    case 64: return type.signedness == Signed ? in->as_i64           : Cast(i64,in->as_u64);
  }
  return 0;
}

typedef struct {
  char const *name;
  TypeInteger type;
  i64 lhs, rhs;
  b32 ok;
  i64 expected;
} ArithCase;

void test_eval_int_add_safe(TestResult *test, void *user) {
  Unused(user);
  ArithCase cases[] = {
    // name                  type                    lhs           rhs  ok      expected
    { "signed_basic",        int_type(Signed, 32),    2,            3,  True,   5 },
    { "unsigned_basic",      int_type(Unsigned, 32),  2,            3,  True,   5 },
    { "signed_overflow",     int_type(Signed, 8),     INT8_MAX,     1,  False,  0 },
    { "signed_underflow",    int_type(Signed, 16),    INT16_MIN,   -1,  False,  0 },
    { "unsigned_overflow",   int_type(Unsigned, 8),   UINT8_MAX,    1,  False,  0 },
    { "signed_64bit",        int_type(Signed, 64),    INT64_MAX,    1,  False,  0 },
  };

  for (usize i = 0; i < Count_of(cases); i++) {
    ArithCase c = cases[i];
    IntBuf lhs = {0}, rhs = {0}, res = {0};
    write_int(c.type, c.lhs, &lhs);
    write_int(c.type, c.rhs, &rhs);
    b32 ok = eval_int_add_safe(c.type, &lhs, &rhs, &res);
    Test_assert_eq(ok, c.ok);
    if (c.ok) { Test_assert_eq(read_int(c.type, &res), c.expected); }
  }
}

void test_eval_int_sub_safe(TestResult *test, void *user) {
  Unused(user);
  ArithCase cases[] = {
    // name                  type                    lhs           rhs  ok      expected
    { "signed_basic",        int_type(Signed, 32),    3,            5,  True,   -2 },
    { "signed_underflow",    int_type(Signed, 8),     INT8_MIN,     1,  False,   0 },
    { "unsigned_underflow",  int_type(Unsigned, 8),   0,            1,  False,   0 },
    { "unsigned_basic",      int_type(Unsigned, 32),  10,           4,  True,    6 },
  };

  for (usize i = 0; i < Count_of(cases); i++) {
    ArithCase c = cases[i];
    IntBuf lhs = {0}, rhs = {0}, res = {0};
    write_int(c.type, c.lhs, &lhs);
    write_int(c.type, c.rhs, &rhs);
    b32 ok = eval_int_sub_safe(c.type, &lhs, &rhs, &res);
    Test_assert_eq(ok, c.ok);
    if (c.ok) { Test_assert_eq(read_int(c.type, &res), c.expected); }
  }
}

void test_eval_int_mul_safe(TestResult *test, void *user) {
  Unused(user);
  ArithCase cases[] = {
    // name                  type                    lhs           rhs  ok      expected
    { "signed_basic",        int_type(Signed, 32),    6,            7,  True,   42 },
    { "signed_overflow",     int_type(Signed, 16),    INT16_MAX,    2,  False,   0 },
    { "unsigned_overflow",   int_type(Unsigned, 16),  UINT16_MAX,   2,  False,   0 },
    { "unsigned_basic",      int_type(Unsigned, 32),  6,            7,  True,   42 },
  };

  for (usize i = 0; i < Count_of(cases); i++) {
    ArithCase c = cases[i];
    IntBuf lhs = {0}, rhs = {0}, res = {0};
    write_int(c.type, c.lhs, &lhs);
    write_int(c.type, c.rhs, &rhs);
    b32 ok = eval_int_mul_safe(c.type, &lhs, &rhs, &res);
    Test_assert_eq(ok, c.ok);
    if (c.ok) { Test_assert_eq(read_int(c.type, &res), c.expected); }
  }
}

typedef struct {
  char const *name;
  TypeInteger type;
  i64 lhs, rhs;
  u32 err;
  i64 expected;
} DivCase;

void test_eval_int_div_safe(TestResult *test, void *user) {
  Unused(user);
  DivCase cases[] = {
    // name                                type                    lhs           rhs  err                          expected
    { "signed_basic",                      int_type(Signed, 32),    10,           3,  IntDivSafe_ok,                3 },
    { "signed_truncates_toward_zero",      int_type(Signed, 32),    -7,           2,  IntDivSafe_ok,               -3 },
    { "unsigned_basic",                    int_type(Unsigned, 32),  20,           4,  IntDivSafe_ok,                5 },
    { "signed_by_zero",                    int_type(Signed, 32),    10,           0,  IntDivSafe_zero_division,     0 },
    { "unsigned_by_zero",                  int_type(Unsigned, 32),  10,           0,  IntDivSafe_zero_division,     0 },
    { "signed_int_min_by_minus_one",       int_type(Signed, 32),    INT32_MIN,   -1,  IntDivSafe_overflow,          0 },
    { "signed_64bit_int_min_by_minus_one", int_type(Signed, 64),    INT64_MIN,   -1,  IntDivSafe_overflow,          0 },
  };

  for (usize i = 0; i < Count_of(cases); i++) {
    DivCase c = cases[i];
    IntBuf lhs = {0}, rhs = {0}, res = {0};
    write_int(c.type, c.lhs, &lhs);
    write_int(c.type, c.rhs, &rhs);
    u32 err = eval_int_div_safe(c.type, &lhs, &rhs, &res);
    Test_assert_eq(err, c.err);
    if (c.err == IntDivSafe_ok) { Test_assert_eq(read_int(c.type, &res), c.expected); }
  }
}

void test_eval_unify(TestResult *test, void *user) {
  Unused(user);

  Arena arena;
  arena_init(&arena, &(ArenaOptions){ .reserve_size = MiB(1), .initial_commit_size = KiB(64) });
  Arena scratch;
  arena_init(&scratch, &(ArenaOptions){ .reserve_size = MiB(1), .initial_commit_size = KiB(64) });

  TypeInterner types = test_types_init(&arena, &scratch);

  TypeIndex t_i32          = types_add(&types, &(Type){ .kind = Type_integer, .data.integer = { .signedness = Signed, .bitwidth = 32 } });
  TypeIndex t_comptime_int = types_add(&types, &(Type){ .kind = Type_comptime_int });
  TypeIndex t_bool         = types_add(&types, &(Type){ .kind = Type_bool });

  TypeIndex t_arr_i32_3      = types_add(&types, &(Type){ .kind = Type_array, .data.array = { .base_type = t_i32, .size = 3 } });
  TypeIndex t_arr_comptime_3 = types_add(&types, &(Type){ .kind = Type_array, .data.array = { .base_type = t_comptime_int, .size = 3 } });
  TypeIndex t_arr_i32_4      = types_add(&types, &(Type){ .kind = Type_array, .data.array = { .base_type = t_i32, .size = 4 } });

  TypeIndex t_slice_i32        = types_add(&types, &(Type){ .kind = Type_slice, .data.slice = { .base_type = t_i32 } });
  TypeIndex t_slice_comptime   = types_add(&types, &(Type){ .kind = Type_slice, .data.slice = { .base_type = t_comptime_int } });
  TypeIndex t_slice_incomplete = types_add(&types, &(Type){ .kind = Type_slice, .data.slice = { .base_type = 0 } });

  TypeIndex t_ptr_i32      = types_add_pointer(&types, t_i32);
  TypeIndex t_ptr_comptime = types_add_pointer(&types, t_comptime_int);

  TypeIndex unified;
  u32 err;

  // Identical array types unify trivially.
  err = eval_unify(&scratch, &types, t_arr_i32_3, t_arr_i32_3, &unified);
  Test_assert_eq(err, UnifyResult_ok);
  Test_assert_eq(unified, t_arr_i32_3);

  // Arrays with a comptime_int element unify element-wise into the concrete integer array.
  err = eval_unify(&scratch, &types, t_arr_comptime_3, t_arr_i32_3, &unified);
  Test_assert_eq(err, UnifyResult_ok);
  Test_assert_eq(Cast(Type *, types_get(&types, unified))->data.array.base_type, t_i32);

  // Arrays of differing size cannot be unified.
  err = eval_unify(&scratch, &types, t_arr_i32_3, t_arr_i32_4, &unified);
  Test_assert_eq(err, UnifyResult_types_cannot_be_unified);

  // Slices unify their base types, same as arrays.
  err = eval_unify(&scratch, &types, t_slice_comptime, t_slice_i32, &unified);
  Test_assert_eq(err, UnifyResult_ok);
  Test_assert_eq(Cast(Type *, types_get(&types, unified))->data.slice.base_type, t_i32);

  // A slice whose base type is still unknown (TypeIndex 0) is itself incomplete.
  err = eval_unify(&scratch, &types, 0, t_slice_incomplete, &unified);
  Test_assert_eq(err, UnifyResult_type_is_incomplete);

  // Pointers unify their base types.
  err = eval_unify(&scratch, &types, t_ptr_comptime, t_ptr_i32, &unified);
  Test_assert_eq(err, UnifyResult_ok);
  Test_assert_eq(Cast(Type *, types_get(&types, unified))->data.pointer.base_type, t_i32);

  // Unrelated kinds cannot be unified.
  err = eval_unify(&scratch, &types, t_bool, t_arr_i32_3, &unified);
  Test_assert_eq(err, UnifyResult_types_cannot_be_unified);

  arena_deinit(&arena);
  arena_deinit(&scratch);
}

void test_is_type_coercible_to(TestResult *test, void *user) {
  Unused(user);

  Arena arena;
  arena_init(&arena, &(ArenaOptions){ .reserve_size = MiB(1), .initial_commit_size = KiB(64) });
  Arena scratch;
  arena_init(&scratch, &(ArenaOptions){ .reserve_size = MiB(1), .initial_commit_size = KiB(64) });

  TypeInterner types = test_types_init(&arena, &scratch);

  TypeIndex t_i32          = types_add(&types, &(Type){ .kind = Type_integer, .data.integer = { .signedness = Signed,   .bitwidth = 32 } });
  TypeIndex t_u8           = types_add(&types, &(Type){ .kind = Type_integer, .data.integer = { .signedness = Unsigned, .bitwidth = 8 } });
  TypeIndex t_comptime_int = types_add(&types, &(Type){ .kind = Type_comptime_int });
  TypeIndex t_bool         = types_add(&types, &(Type){ .kind = Type_bool });

  TypeIndex t_fn_i32  = types_add(&types, &(Type){ .kind = Type_function, .data.function = { .return_type = t_i32,  .param_count = 0 } });
  TypeIndex t_fn_bool = types_add(&types, &(Type){ .kind = Type_function, .data.function = { .return_type = t_bool, .param_count = 0 } });

  Test_assert_eq(is_type_coercible_to(&types, t_i32, t_i32), True);
  Test_assert_eq(is_type_coercible_to(&types, t_i32, t_comptime_int), True);
  Test_assert_eq(is_type_coercible_to(&types, t_fn_i32, t_fn_i32), True);
  Test_assert_eq(is_type_coercible_to(&types, t_fn_i32, t_fn_bool), False);
  Test_assert_eq(is_type_coercible_to(&types, t_i32, t_bool), True);
  Test_assert_eq(is_type_coercible_to(&types, t_i32, t_u8), False);

  arena_deinit(&arena);
  arena_deinit(&scratch);
}

void register_eval_tests(TestRunner *runner) {
#define EvalTest(function) \
  test_runner_register_test(runner, string_lit(#function), function, Null)

  EvalTest(test_eval_int_add_safe);
  EvalTest(test_eval_int_sub_safe);
  EvalTest(test_eval_int_mul_safe);
  EvalTest(test_eval_int_div_safe);
  EvalTest(test_eval_unify);
  EvalTest(test_is_type_coercible_to);

#undef EvalTest
}
