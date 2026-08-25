#include "toteload.h"
#include "blu.h"
#include "types.h"
#include "value.h"
#include "eval.h"
#include "test.h"

internal TypeInteger int_type(u8 signedness, u16 bitwidth) {
  return (TypeInteger){ .signedness = signedness, .bitwidth = bitwidth };
}

// -- eval_int_add ---------------------------------------------------------

void test_add_signed_basic(TestResult *test, void *user) {
  Unused(user);
  i32 lhs = 2, rhs = 3, res = 0;
  b32 ok = eval_int_add(int_type(Signed, 32), &lhs, &rhs, &res);
  Test_assert(ok);
  Test_assert_eq(res, 5);
}

void test_add_unsigned_basic(TestResult *test, void *user) {
  Unused(user);
  u32 lhs = 2, rhs = 3, res = 0;
  b32 ok = eval_int_add(int_type(Unsigned, 32), &lhs, &rhs, &res);
  Test_assert(ok);
  Test_assert_eq(res, 5u);
}

void test_add_signed_overflow(TestResult *test, void *user) {
  Unused(user);
  i8 lhs = INT8_MAX, rhs = 1, res = 0;
  b32 ok = eval_int_add(int_type(Signed, 8), &lhs, &rhs, &res);
  Test_assert(!ok);
}

void test_add_signed_underflow(TestResult *test, void *user) {
  Unused(user);
  i16 lhs = INT16_MIN, rhs = -1, res = 0;
  b32 ok = eval_int_add(int_type(Signed, 16), &lhs, &rhs, &res);
  Test_assert(!ok);
}

void test_add_unsigned_overflow(TestResult *test, void *user) {
  Unused(user);
  u8 lhs = UINT8_MAX, rhs = 1, res = 0;
  b32 ok = eval_int_add(int_type(Unsigned, 8), &lhs, &rhs, &res);
  Test_assert(!ok);
}

void test_add_signed_64bit(TestResult *test, void *user) {
  Unused(user);
  i64 lhs = INT64_MAX, rhs = 1, res = 0;
  b32 ok = eval_int_add(int_type(Signed, 64), &lhs, &rhs, &res);
  Test_assert(!ok);
}

// -- eval_int_sub -----------------------------------------------------------

void test_sub_signed_basic(TestResult *test, void *user) {
  Unused(user);
  i32 lhs = 3, rhs = 5, res = 0;
  b32 ok = eval_int_sub(int_type(Signed, 32), &lhs, &rhs, &res);
  Test_assert(ok);
  Test_assert_eq(res, -2);
}

void test_sub_signed_underflow(TestResult *test, void *user) {
  Unused(user);
  i8 lhs = INT8_MIN, rhs = 1, res = 0;
  b32 ok = eval_int_sub(int_type(Signed, 8), &lhs, &rhs, &res);
  Test_assert(!ok);
}

void test_sub_unsigned_underflow(TestResult *test, void *user) {
  Unused(user);
  u8 lhs = 0, rhs = 1, res = 0;
  b32 ok = eval_int_sub(int_type(Unsigned, 8), &lhs, &rhs, &res);
  Test_assert(!ok);
}

void test_sub_unsigned_basic(TestResult *test, void *user) {
  Unused(user);
  u32 lhs = 10, rhs = 4, res = 0;
  b32 ok = eval_int_sub(int_type(Unsigned, 32), &lhs, &rhs, &res);
  Test_assert(ok);
  Test_assert_eq(res, 6u);
}

// -- eval_int_mul -----------------------------------------------------------

void test_mul_signed_basic(TestResult *test, void *user) {
  Unused(user);
  i32 lhs = 6, rhs = 7, res = 0;
  b32 ok = eval_int_mul(int_type(Signed, 32), &lhs, &rhs, &res);
  Test_assert(ok);
  Test_assert_eq(res, 42);
}

void test_mul_signed_overflow(TestResult *test, void *user) {
  Unused(user);
  i16 lhs = INT16_MAX, rhs = 2, res = 0;
  b32 ok = eval_int_mul(int_type(Signed, 16), &lhs, &rhs, &res);
  Test_assert(!ok);
}

void test_mul_unsigned_overflow(TestResult *test, void *user) {
  Unused(user);
  u16 lhs = UINT16_MAX, rhs = 2, res = 0;
  b32 ok = eval_int_mul(int_type(Unsigned, 16), &lhs, &rhs, &res);
  Test_assert(!ok);
}

void test_mul_unsigned_basic(TestResult *test, void *user) {
  Unused(user);
  u32 lhs = 6, rhs = 7, res = 0;
  b32 ok = eval_int_mul(int_type(Unsigned, 32), &lhs, &rhs, &res);
  Test_assert(ok);
  Test_assert_eq(res, 42u);
}

// -- eval_int_div -----------------------------------------------------------

void test_div_signed_basic(TestResult *test, void *user) {
  Unused(user);
  i32 lhs = 10, rhs = 3, res = 0;
  b32 ok = eval_int_div(int_type(Signed, 32), &lhs, &rhs, &res);
  Test_assert(ok);
  Test_assert_eq(res, 3);
}

void test_div_signed_truncates_toward_zero(TestResult *test, void *user) {
  Unused(user);
  i32 lhs = -7, rhs = 2, res = 0;
  b32 ok = eval_int_div(int_type(Signed, 32), &lhs, &rhs, &res);
  Test_assert(ok);
  Test_assert_eq(res, -3);
}

void test_div_unsigned_basic(TestResult *test, void *user) {
  Unused(user);
  u32 lhs = 20, rhs = 4, res = 0;
  b32 ok = eval_int_div(int_type(Unsigned, 32), &lhs, &rhs, &res);
  Test_assert(ok);
  Test_assert_eq(res, 5u);
}

void test_div_signed_by_zero(TestResult *test, void *user) {
  Unused(user);
  i32 lhs = 10, rhs = 0, res = 0;
  b32 ok = eval_int_div(int_type(Signed, 32), &lhs, &rhs, &res);
  Test_assert(!ok);
}

void test_div_unsigned_by_zero(TestResult *test, void *user) {
  Unused(user);
  u32 lhs = 10, rhs = 0, res = 0;
  b32 ok = eval_int_div(int_type(Unsigned, 32), &lhs, &rhs, &res);
  Test_assert(!ok);
}

void test_div_signed_int_min_by_minus_one(TestResult *test, void *user) {
  Unused(user);
  i32 lhs = INT32_MIN, rhs = -1, res = 0;
  b32 ok = eval_int_div(int_type(Signed, 32), &lhs, &rhs, &res);
  Test_assert(!ok);
}

void test_div_signed_64bit_int_min_by_minus_one(TestResult *test, void *user) {
  Unused(user);
  i64 lhs = INT64_MIN, rhs = -1, res = 0;
  b32 ok = eval_int_div(int_type(Signed, 64), &lhs, &rhs, &res);
  Test_assert(!ok);
}

void register_eval_tests(TestRunner *runner) {
#define EvalTest(function) \
  test_runner_register_test(runner, string_lit(#function), function, Null)

  EvalTest(test_add_signed_basic);
  EvalTest(test_add_unsigned_basic);
  EvalTest(test_add_signed_overflow);
  EvalTest(test_add_signed_underflow);
  EvalTest(test_add_unsigned_overflow);
  EvalTest(test_add_signed_64bit);

  EvalTest(test_sub_signed_basic);
  EvalTest(test_sub_signed_underflow);
  EvalTest(test_sub_unsigned_underflow);
  EvalTest(test_sub_unsigned_basic);

  EvalTest(test_mul_signed_basic);
  EvalTest(test_mul_signed_overflow);
  EvalTest(test_mul_unsigned_overflow);
  EvalTest(test_mul_unsigned_basic);

  EvalTest(test_div_signed_basic);
  EvalTest(test_div_signed_truncates_toward_zero);
  EvalTest(test_div_unsigned_basic);
  EvalTest(test_div_signed_by_zero);
  EvalTest(test_div_unsigned_by_zero);
  EvalTest(test_div_signed_int_min_by_minus_one);
  EvalTest(test_div_signed_64bit_int_min_by_minus_one);

#undef EvalTest
}
