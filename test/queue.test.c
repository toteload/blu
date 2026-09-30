#include "toteload.h"
#include "test.h"

#include <stdlib.h>

#define QUEUE_NAME            U32Queue
#define QUEUE_TYPE            u32
#define QUEUE_MIN_SIZE_LOG2   2
#define QUEUE_FUNCTION_PREFIX u32_queue
#define QUEUE_LINKAGE         internal
#define QUEUE_OUTPUT_TYPES
#define QUEUE_OUTPUT_DEFINITIONS
#include "queue.h"

internal void *queue_test_alloc_fn(void *ctx, void *p, usize old_byte_size, usize new_byte_size, u32 align) {
  Unused(ctx, old_byte_size, align);

  if (new_byte_size == 0) {
    free(p);
    return Null;
  }

  return realloc(p, new_byte_size);
}

void test_queue_fifo(TestResult *test, void *user) {
  Unused(user);
  Allocator allocator = { .fn = queue_test_alloc_fn };

  U32Queue queue = {0};

  for (u32 i = 0; i < 10; i++) {
    u32_queue_append(&queue, allocator, i);
  }

  Test_assert_eq(queue.len, 10);
  Test_assert_eq(queue.cap, 16);

  for (u32 i = 0; i < 10; i++) {
    Test_assert_eq(u32_queue_pop(&queue), i);
  }

  Test_assert_eq(queue.len, 0);

  u32_queue_deinit(&queue, allocator);
}

void test_queue_grow_while_wrapped(TestResult *test, void *user) {
  Unused(user);
  Allocator allocator = { .fn = queue_test_alloc_fn };

  U32Queue queue = {0};

  // Fill the initial 4 slots, pop 3 and push 3 so the items wrap around the
  // end of the buffer, then push past capacity to force a grow.
  for (u32 i = 0; i < 4; i++) {
    u32_queue_append(&queue, allocator, i);
  }

  for (u32 i = 0; i < 3; i++) {
    Test_assert_eq(u32_queue_pop(&queue), i);
  }

  for (u32 i = 4; i < 7; i++) {
    u32_queue_append(&queue, allocator, i);
  }

  Test_assert_eq(queue.cap, 4);
  Test_assert_eq(queue.head, 3);

  for (u32 i = 7; i < 12; i++) {
    u32_queue_append(&queue, allocator, i);
  }

  Test_assert_eq(queue.cap, 16);

  for (u32 i = 0; i < queue.len; i++) {
    Test_assert_eq(u32_queue_at_unchecked(&queue, i), 3 + i);
  }

  for (u32 i = 3; i < 12; i++) {
    Test_assert_eq(u32_queue_pop(&queue), i);
  }

  Test_assert_eq(queue.len, 0);

  u32_queue_deinit(&queue, allocator);
}

void register_queue_tests(TestRunner *runner) {
  test_runner_register_test(runner, string_lit("test_queue_fifo"), test_queue_fifo, Null);
  test_runner_register_test(runner, string_lit("test_queue_grow_while_wrapped"), test_queue_grow_while_wrapped, Null);
}
