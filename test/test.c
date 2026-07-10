#include "test.h"
#include <stdio.h>

#define SEGMENTLIST_NAME            TestList
#define SEGMENTLIST_TYPE            Test
#define SEGMENTLIST_FUNCTION_PREFIX list
#define SEGMENTLIST_MIN_SIZE_LOG2   TestList_min_size_log2
#define SEGMENTLIST_SEGMENT_COUNT   TestList_segment_count
#define SEGMENTLIST_LINKAGE         internal 
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

void test_runner_init(TestRunner *runner) {
  arena_init(&runner->arena, &(ArenaOptions){
    .reserve_size        = MiB(64),
    .initial_commit_size = KiB(64),
  });
}

void test_runner_deinit(TestRunner *runner) {
  arena_deinit(&runner->arena);
}

void test_runner_register_test(TestRunner *runner, String name, FnTest fn, void *user) {
  String s = arena_copy_string(&runner->arena, name);
  list_append(&runner->tests, &runner->arena, (Test){ .name = s, .fn = fn, .user = user });
}

void test_runner_run(TestRunner *runner) {
  u32 total_count = runner->tests.len;
  u32 fail_count = 0;

  for (u32 i = 0; i < total_count; i++) {
    Test test = list_at_unchecked(&runner->tests, i);

    TestResult result = {0};
    test.fn(&result, test.user);

    printf("%s %.*s",
      (result.failed) ? "[FAIL]" : "[PASS]",
      Cast(int, test.name.len), test.name.str
    );

    if (result.failed) {
      fail_count += 1;
      printf(" at %s:%u\n", result.filename, result.line);
      printf("     - %.*s\n", Cast(int, result.reason.len), result.reason.str);
    } else {
      printf("\n");
    }
  }

  u32 pass_count = total_count - fail_count;

  printf("\nSummary: %u/%u test(s) passed (%2.2f%%)\n", pass_count, total_count, 100.0f * Cast(f32, pass_count) / total_count);
}
