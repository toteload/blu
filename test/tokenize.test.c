#include "toteload.h"
#include "blu.h"
#include "tokens.h"
#include "test.h"

typedef void (*FnTokenizerTest)(TestResult*, TokenizeContext*);

void dummy_add_message(void *user, u8 severity, MessageLocation location, String format, ...) { 
  Unused(user, severity, location, format);
}

void tokenizer_test(TestResult *test, void *user) {
  FnTokenizerTest fn = Cast(FnTokenizerTest, user);

  MessageSink sink = {
    .user = Null,
    .add_message = dummy_add_message,
  };

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

// Tokenize `src`, assert success, and assert the exact kind sequence.
#define Assert_kinds(src, ...)                                                  \
  do {                                                                          \
    u8 expected[] = { __VA_ARGS__ };                                            \
    Tokens toks;                                                                \
    b32 ok = tokenize(context, string_lit(src), &toks);                         \
    Test_assert(ok);                                                            \
    Test_assert_eq(toks.tok_count, Count_of(expected));                         \
    for (u32 i = 0; i < Count_of(expected); i++) {                             \
      Test_assert_eq(toks.kinds[i], expected[i]);                               \
    }                                                                           \
  } while (0)

void test_single_char_tokens(TestResult *test, TokenizeContext *context) {
  Assert_kinds(",.:{}()[]",
    Tok_comma, Tok_dot, Tok_colon,
    Tok_brace_open, Tok_brace_close,
    Tok_paren_open, Tok_paren_close,
    Tok_bracket_open, Tok_bracket_close);
  Assert_kinds("* / % & | ^ ~",
    Tok_star, Tok_slash, Tok_percent,
    Tok_ampersand, Tok_bar, Tok_caret, Tok_tilde);
}

void test_multichar_operators(TestResult *test, TokenizeContext *context) {
  Assert_kinds("->",  Tok_arrow);
  Assert_kinds("==",  Tok_cmp_eq);
  Assert_kinds("=",   Tok_equals);
  Assert_kinds("+=",  Tok_plus_equals);
  Assert_kinds("+",   Tok_plus);
  Assert_kinds("<=",  Tok_cmp_le);
  Assert_kinds("<<",  Tok_left_shift);
  Assert_kinds("<",   Tok_cmp_lt);
  Assert_kinds(">=",  Tok_cmp_ge);
  Assert_kinds(">>",  Tok_right_shift);
  Assert_kinds(">",   Tok_cmp_gt);
  Assert_kinds("!=",  Tok_cmp_ne);
  Assert_kinds("!",   Tok_exclamation);
  Assert_kinds("-",   Tok_minus);
}

void test_literals(TestResult *test, TokenizeContext *context) {
  Assert_kinds("0",          Tok_literal_int);
  Assert_kinds("12345",      Tok_literal_int);
  Assert_kinds("\"hi\"",     Tok_literal_string);
  Assert_kinds("\"a\\\"b\"", Tok_literal_string);     // escaped quote inside string
}

void test_keywords_and_identifiers(TestResult *test, TokenizeContext *context) {
  Assert_kinds("foo _bar baz123",
    Tok_identifier, Tok_identifier, Tok_identifier);
  Assert_kinds("if else for do break continue return",
    Tok_keyword_if, Tok_keyword_else, Tok_keyword_for, Tok_keyword_do,
    Tok_keyword_break, Tok_keyword_continue, Tok_keyword_return);
  Assert_kinds("and or defer const cast bitcast as mod no_cache inline",
    Tok_keyword_and, Tok_keyword_or, Tok_keyword_defer, Tok_keyword_const,
    Tok_keyword_cast, Tok_keyword_bitcast, Tok_keyword_as, Tok_keyword_mod,
    Tok_keyword_no_cache, Tok_keyword_inline);
  Assert_kinds("#print",  Tok_builtin_print);
  Assert_kinds("returns", Tok_identifier);   // keyword is a prefix, not a full match
}

void test_newlines_and_comments_dropped(TestResult *test, TokenizeContext *context) {
  Tokens toks;
  b32 ok = tokenize(context, string_lit("a\n; comment\nb"), &toks);
  Test_assert(ok);
  Test_assert_eq(toks.tok_count, 2);          // only `a` and `b`
  Test_assert_eq(toks.kinds[0], Tok_identifier);
  Test_assert_eq(toks.kinds[1], Tok_identifier);
  Test_assert(toks.line_count > 1);           // lines were recorded
}

void test_whitespace_skipped(TestResult *test, TokenizeContext *context) {
  Assert_kinds("  \t a\t=\r 1 ",
    Tok_identifier, Tok_equals, Tok_literal_int);
}

void test_spans(TestResult *test, TokenizeContext *context) {
  Tokens toks;
  b32 ok = tokenize(context, string_lit("foo == 42"), &toks);
  Test_assert(ok);
  Test_assert(toks.tok_count >= 3);
  Test_assert_eq(toks.spans[0].start, 0);   // foo
  Test_assert_eq(toks.spans[0].end,   3);
  Test_assert_eq(toks.spans[1].start, 4);   // ==
  Test_assert_eq(toks.spans[1].end,   6);
  Test_assert_eq(toks.spans[2].start, 7);   // 42
  Test_assert_eq(toks.spans[2].end,   9);
}

void test_error_cases(TestResult *test, TokenizeContext *context) {
  Tokens toks;
  Test_assert(!tokenize(context, string_lit("\"unterminated"), &toks));
  Test_assert(!tokenize(context, string_lit("$"), &toks));
}

void test_declaration_snippet(TestResult *test, TokenizeContext *context) {
  Assert_kinds("x : i32 = 42",
    Tok_identifier, Tok_colon, Tok_identifier, Tok_equals, Tok_literal_int);
}

void test_string_escapes(TestResult *test, TokenizeContext *context) {
  Assert_kinds("\"\"",     Tok_literal_string);   // empty string
  Assert_kinds("\"\\\\\"", Tok_literal_string);   // "\\"  — escaped backslash
  Assert_kinds("\"\\n\"",  Tok_literal_string);   // "\n"
}

void test_no_whitespace(TestResult *test, TokenizeContext *context) {
  Assert_kinds("x:i32=42",
    Tok_identifier, Tok_colon, Tok_identifier, Tok_equals, Tok_literal_int);
}

void test_operator_munch_boundaries(TestResult *test, TokenizeContext *context) {
  Assert_kinds("===", Tok_cmp_eq,      Tok_equals);
  Assert_kinds("+==", Tok_plus_equals, Tok_equals);
  Assert_kinds("!==", Tok_cmp_ne,      Tok_equals);
  Assert_kinds("->>", Tok_arrow,       Tok_cmp_gt);
  Assert_kinds("<<<", Tok_left_shift,  Tok_cmp_lt);
}

void test_number_then_identifier(TestResult *test, TokenizeContext *context) {
  Assert_kinds("123abc", Tok_literal_int, Tok_identifier);
}

void test_string_span(TestResult *test, TokenizeContext *context) {
  Tokens toks;
  b32 ok = tokenize(context, string_lit("\"hi\""), &toks);
  Test_assert(ok);
  Test_assert(toks.tok_count >= 1);
  Test_assert_eq(toks.spans[0].start, 0);
  Test_assert_eq(toks.spans[0].end,   4);   // includes both quotes
}

typedef struct {
  String          name;
  FnTokenizerTest fn;
} TokenizeTest;

#define Test(f) { .name = string_lit(#f), .fn = f }

void register_tokenizer_tests(TestRunner *runner) {
  TokenizeTest tests[] = {
    Test(test_empty_input),
    Test(test_single_char_tokens),
    Test(test_multichar_operators),
    Test(test_literals),
    Test(test_keywords_and_identifiers),
    Test(test_newlines_and_comments_dropped),
    Test(test_whitespace_skipped),
    Test(test_spans),
    Test(test_error_cases),
    Test(test_declaration_snippet),
    Test(test_string_escapes),
    Test(test_no_whitespace),
    Test(test_operator_munch_boundaries),
    Test(test_number_then_identifier),
    Test(test_string_span),
  };

  for (u32 i = 0; i < Count_of(tests); i++) {
    test_runner_register_test(runner, tests[i].name, Cast(FnTest, tokenizer_test), Cast(void*, tests[i].fn));
  }
}
