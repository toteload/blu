#include "toteload.h"
#include "blu.h"
#include "tokens.h"
#include "messages.h"
#include "test.h"

typedef struct {
  Arena *arena;
  Arena *scratch;
} MessagesTestContext;

typedef void (*FnMessagesTest)(TestResult *, MessagesTestContext *);

void messages_test(TestResult *test, void *user) {
  FnMessagesTest fn = Cast(FnMessagesTest, user);

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

  MessagesTestContext context = {
    .arena   = &arena,
    .scratch = &scratch,
  };

  fn(test, &context);

  arena_deinit(&arena);
  arena_deinit(&scratch);
}

// Builds a Message with the given format and args and returns its substituted text.
internal String format_with_args(MessagesTestContext *ctx, String format, MessageArg *args, u32 arg_count) {
  Message *msg = arena_push(ctx->arena, sizeof(Message) + arg_count * sizeof(MessageArg), Align_of(Message));
  msg->severity = Severity_Error;
  msg->location = (MessageLocation){ .kind = MessageLocation_unspecified };
  msg->format   = format;
  for (u32 i = 0; i < arg_count; i++) {
    msg->args[i] = args[i];
  }
  return message_format(ctx->scratch, msg);
}

void test_no_args(TestResult *test, MessagesTestContext *ctx) {
  String out = format_with_args(ctx, string_lit("plain message"), Null, 0);
  Test_assert(string_eq(out, string_lit("plain message")));
}

void test_string_arg(TestResult *test, MessagesTestContext *ctx) {
  MessageArg args[1] = { { .string = string_lit("foo.blu") } };
  String out = format_with_args(ctx, string_lit("Could not open/read file %string."), args, 1);
  Test_assert(string_eq(out, string_lit("Could not open/read file foo.blu.")));
}

void test_tokenkind_arg(TestResult *test, MessagesTestContext *ctx) {
  MessageArg args[2] = {
    { .token_kind = Tok_identifier },
    { .token_kind = Tok_paren_close },
  };
  String out = format_with_args(
    ctx, string_lit("Expected token %tokenkind, but got token %tokenkind."), args, 2
  );
  Test_assert(string_eq(out, string_lit("Expected token identifier, but got token ).")));
}

void test_percent_literal(TestResult *test, MessagesTestContext *ctx) {
  String out = format_with_args(ctx, string_lit("100%% done"), Null, 0);
  Test_assert(string_eq(out, string_lit("100% done")));
}


void register_messages_tests(TestRunner *runner) {
#define MessagesTest(function) \
  test_runner_register_test(runner, string_lit(#function), messages_test, Cast(void*, function))

  MessagesTest(test_no_args);
  MessagesTest(test_string_arg);
  MessagesTest(test_tokenkind_arg);
  MessagesTest(test_percent_literal);

#undef MessagesTest
}
