#ifndef TEST_H
#define TEST_H

#include "toteload.h"
#include <stdio.h>

#define Test_reason_buf_size 512

typedef struct {
  String reason;
  char const *filename;
  u32 line;
  u8 failed;
  u8 buf[Test_reason_buf_size];
} TestResult;

typedef void (*FnTest)(TestResult *result, void *user);

typedef struct {
  String  name;
  FnTest  fn;
  void   *user;
} Test;

#define TestList_min_size_log2      8
#define TestList_segment_count      24
#define SEGMENTLIST_NAME            TestList
#define SEGMENTLIST_TYPE            Test
#define SEGMENTLIST_FUNCTION_PREFIX list
#define SEGMENTLIST_MIN_SIZE_LOG2   TestList_min_size_log2
#define SEGMENTLIST_SEGMENT_COUNT   TestList_segment_count
#define SEGMENTLIST_LINKAGE         internal 
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

typedef struct {
  Arena arena;
  TestList tests;
} TestRunner;

void test_runner_init(TestRunner *runner);
void test_runner_deinit(TestRunner *runner);
void test_runner_register_test(TestRunner *runner, String name, FnTest fn, void *user);
void test_runner_run(TestRunner *runner);

#define Test_assert(e) do { if (!(e)) { test->line = __LINE__; test->filename = __FILE__; \
    test->failed = True; \
    i32 n = snprintf(Cast(char*,test->buf), Test_reason_buf_size, "Assertion failed: %s\n", #e); \
    test->reason = (String){ .len = Cast(usize,n-1), .str = test->buf }; \
    return; } } while (0)

#define Test_assert_eq(a,b) do { if ((a) != (b)) { test->line = __LINE__; test->filename = __FILE__; \
    test->failed = True; \
    i32 n = snprintf(Cast(char*,test->buf), Test_reason_buf_size, "Assertion failed: %s == %s\n", #a, #b); \
    test->reason = (String){ .len = Cast(usize,n-1), .str = test->buf }; \
    return; } } while (0)

#endif // TEST_H
