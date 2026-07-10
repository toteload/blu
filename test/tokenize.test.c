#include "toteload.h"
#include "blu.h"
#include "tokens.h"
#include "test.h"

typedef void (*FnTokenizerTest)(TestResult*, TokenizeContext*);

void tokenizer_test(TestResult *test, FnTokenizerTest fn) {
  MessageSink sink = {0}; // TODO init this

  Arena arena;
  arena_init(&arena, &(ArenaOptions){
    .reserve_size        = MiB(1),
    .initial_commit_size = KiB(64),
  });

  Arena scratch;
  arena_init(&scratch, &(ArenaOptions){
    .reserve_size        = MiB(1),
    .initial_commit_size = KiB(64),
  });

  TokenizeContext context = {
    .msg_sink = &sink,
    .arena    = &arena,
    .scratch  = &scratch,
  };

  fn(test, &context);

  arena_deinit(&arena);
  arena_deinit(&scratch);
}

void test_empty_input(TestResult *test, TokenizeContext *context) {
  Tokens toks;
  b32 ok = tokenize(context, string_lit(""), &toks);

  Test_assert(ok);
  Test_assert_eq(toks.tok_count, 0);
}

typedef struct {
  String          name;
  FnTokenizerTest fn;
} TokenizeTest;

#define Test(f) { .name = string_lit(#f), .fn = f }

void register_tokenizer_tests(TestRunner *runner) {
  TokenizeTest tests[] = {
    Test(test_empty_input),
  };

  for (u32 i = 0; i < Count_of(tests); i++) {
    test_runner_register_test(runner, tests[i].name, Cast(FnTest, tokenizer_test), tests[i].fn);
  }
}
