#include "toteload.h"
#include "blu.h"
#include "test.h"

#include <stdio.h>

void test_assert(TestResult *test, void *user) {
  Test_assert(True);
}

void test_assert_eq(TestResult *test, void *user) {
  Test_assert_eq(1, 1);
}

#define Test(function) { .name = string_lit(#function), .f = function }

extern void register_tokenizer_tests(TestRunner *runner);

int main(void) {
  TestRunner runner;
  test_runner_init(&runner);

  register_tokenizer_tests(&runner);

  test_runner_register_test(&runner, string_lit("test_assert_eq"), test_assert_eq, Null);
  test_runner_register_test(&runner, string_lit("test_assert"), test_assert, Null);

  test_runner_run(&runner);
  
  test_runner_deinit(&runner);

  return 0;
}
