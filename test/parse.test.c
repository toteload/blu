#include "toteload.h"
#include "blu.h"
#include "tokens.h"
#include "ast.h"
#include "test.h"

typedef struct {
  MessageSink *msg_sink;
  Arena       *arena;
  Arena       *scratch;
  Tokens       tokens;
} ParserTestContext;

typedef void (*FnParserTest)(TestResult *, ParserTestContext *);

void parser_dummy_add_message(void *user, u8 severity, MessageLocation location, String format, ...) { 
  Unused(user, severity, location, format);
}

void parser_test(TestResult *test, void *user) {
  FnParserTest fn = Cast(FnParserTest, user);
  MessageSink sink = {
    .user        = Null,
    .add_message = parser_dummy_add_message,
  };

  Arena arena;
  arena_init(&arena, &(ArenaOptions){
    .reserve_size        = MiB(4),
    .initial_commit_size = KiB(64),
  });

  Arena scratch;
  arena_init(&scratch, &(ArenaOptions){
    .reserve_size        = MiB(4),
    .initial_commit_size = KiB(64),
  });

  ParserTestContext context = {
    .msg_sink = &sink,
    .arena    = &arena,
    .scratch  = &scratch,
  };

  fn(test, &context);

  arena_deinit(&arena);
  arena_deinit(&scratch);
}

// Tokenize then parse `src`. The root node is always at index 0. The tokens are stored in the
// context so tests can inspect them (e.g. via token_string) after parsing.
internal b32 do_parse(ParserTestContext *context, String src, AstNodes *ast) {
  TokenizeContext tokenize_context = {
    .msg_sink = context->msg_sink,
    .arena    = context->arena,
    .scratch  = context->scratch,
  };

  if (!tokenize(&tokenize_context, src, &context->tokens)) {
    return False;
  }

  ParseContext parse_context = {
    .msg_sink = context->msg_sink,
    .arena    = context->arena,
    .scratch  = context->scratch,
  };

  return parse(&parse_context, &context->tokens, ast);
}

void test_parse_empty(TestResult *test, ParserTestContext *context) {
  AstNodes ast;
  b32 ok = do_parse(context, string_lit(""), &ast);

  Test_assert(ok);
  Test_assert_eq(ast_kind(&ast, AST_KEY_SOURCE), Ast_source);

  AstSource *source = ast_data(&ast, AST_KEY_SOURCE);
  Test_assert_eq(source->count, 0);
}

void test_parse_mod_section(TestResult *test, ParserTestContext *context) {
  String text = string_lit("mod main");

  AstNodes ast;
  b32 ok = do_parse(context, text, &ast);

  Test_assert(ok);
  Test_assert_eq(ast_kind(&ast, AST_KEY_SOURCE), Ast_source);

  AstSource *source = ast_data(&ast, AST_KEY_SOURCE);
  Test_assert_eq(source->count, 1);

  AstKey mod_idx = source->items[0];
  Test_assert_eq(ast_kind(&ast, mod_idx), Ast_mod_section);

  AstModSection *mod = ast_data(&ast, mod_idx);
  Test_assert_eq(mod->count, 0);

  Test_assert(string_eq(token_string(&context->tokens, text, mod->name), string_lit("main")));
}

void test_parse_declaration(TestResult *test, ParserTestContext *context) {
  AstNodes ast;
  b32 ok = do_parse(context, string_lit("mod main\nx : i32 = 42"), &ast);

  Test_assert(ok);

  AstSource     *source = ast_data(&ast, AST_KEY_SOURCE);
  AstModSection *mod    = ast_data(&ast, source->items[0]);
  Test_assert_eq(mod->count, 1);

  AstKey decl_idx = mod->items[0];
  Test_assert_eq(ast_kind(&ast, decl_idx), Ast_declaration);

  AstDeclaration *decl = ast_data(&ast, decl_idx);
  Test_assert_eq(ast_kind(&ast, decl->type), Ast_identifier);
  Test_assert_eq(ast_kind(&ast, decl->value), Ast_literal_int);
}

void test_parse_declaration_without_type(TestResult *test, ParserTestContext *context) {
  AstNodes ast;
  b32 ok = do_parse(context, string_lit("mod main\nx : = 42"), &ast);

  Test_assert(ok);

  AstSource      *source = ast_data(&ast, AST_KEY_SOURCE);
  AstModSection  *mod    = ast_data(&ast, source->items[0]);
  AstDeclaration *decl   = ast_data(&ast, mod->items[0]);

  Test_assert(key_is_nil(decl->type));   // no type given
  Test_assert_eq(ast_kind(&ast, decl->value), Ast_literal_int);
}

void test_parse_binary_precedence(TestResult *test, ParserTestContext *context) {
  // `1 + 2 * 3` should parse as `1 + (2 * 3)`.
  AstNodes ast;
  b32 ok = do_parse(context, string_lit("mod main\nx : i32 = 1 + 2 * 3"), &ast);

  Test_assert(ok);

  AstSource      *source = ast_data(&ast, AST_KEY_SOURCE);
  AstModSection  *mod    = ast_data(&ast, source->items[0]);
  AstDeclaration *decl   = ast_data(&ast, mod->items[0]);

  Test_assert_eq(ast_kind(&ast, decl->value), Ast_binary_op);

  AstBinaryOp *add = ast_data(&ast, decl->value);
  Test_assert_eq(add->op_kind, Add);
  Test_assert_eq(ast_kind(&ast, add->lhs), Ast_literal_int);
  Test_assert_eq(ast_kind(&ast, add->rhs), Ast_binary_op);

  AstBinaryOp *mul = ast_data(&ast, add->rhs);
  Test_assert_eq(mul->op_kind, Mul);
  Test_assert_eq(ast_kind(&ast, mul->lhs), Ast_literal_int);
  Test_assert_eq(ast_kind(&ast, mul->rhs), Ast_literal_int);
}

void test_parse_function(TestResult *test, ParserTestContext *context) {
  AstNodes ast;
  b32 ok = do_parse(context, string_lit("mod main\nadd : (i32, i32) i32 = |a, b| { a + b }"), &ast);

  Test_assert(ok);

  AstSource      *source = ast_data(&ast, AST_KEY_SOURCE);
  AstModSection  *mod    = ast_data(&ast, source->items[0]);
  AstDeclaration *decl   = ast_data(&ast, mod->items[0]);

  Test_assert_eq(ast_kind(&ast, decl->type), Ast_type_function);
  Test_assert_eq(ast_kind(&ast, decl->value), Ast_function);

  AstFunction *fn = ast_data(&ast, decl->value);
  Test_assert_eq(fn->count, 2);
  Test_assert_eq(ast_kind(&ast, fn->params[0]), Ast_param);
  Test_assert_eq(ast_kind(&ast, fn->params[1]), Ast_param);
  Test_assert_eq(ast_kind(&ast, fn->body), Ast_block);
}

void test_parse_missing_mod_name(TestResult *test, ParserTestContext *context) {
  AstNodes ast;
  b32 ok = do_parse(context, string_lit("mod"), &ast);
  Test_assert(!ok);
}

void test_parse_missing_value(TestResult *test, ParserTestContext *context) {
  AstNodes ast;
  b32 ok = do_parse(context, string_lit("mod main\nx : i32 ="), &ast);
  Test_assert(!ok);
}

typedef struct {
  String       name;
  FnParserTest fn;
} ParserTest;

#define Test(f) { .name = string_lit(#f), .fn = f }

void register_parser_tests(TestRunner *runner) {
  ParserTest tests[] = {
    Test(test_parse_empty),
    Test(test_parse_mod_section),
    Test(test_parse_declaration),
    Test(test_parse_declaration_without_type),
    Test(test_parse_binary_precedence),
    Test(test_parse_function),
    Test(test_parse_missing_mod_name),
    Test(test_parse_missing_value),
  };

  for (u32 i = 0; i < Count_of(tests); i++) {
    test_runner_register_test(runner, tests[i].name, Cast(FnTest, parser_test), Cast(void *, tests[i].fn));
  }
}
