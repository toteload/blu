#include "blu.hh"

struct Parser {
  StringInterner *strings;
  Vector<AstNode> *nodes;
  Slice<Token> tokens;

  Allocator ref_allocator;
  usize idx;

  void init(StringInterner *strings, Allocator ref_allocator, Vector<AstNode> *nodes, Slice<Token> tokens);

  b32 parse_module(AstRef *out);

  b32 parse_item(AstRef *out);
  b32 parse_item_function(AstRef *out);

  b32 parse_type(AstRef *out);

  b32 parse_statement(AstRef *out);
  b32 parse_statement_return(AstRef *out);
  b32 parse_declaration(AstRef *out);
  b32 parse_literal_int(AstRef *out);
  b32 parse_identifier(AstRef *out);

  b32 parse_expression(AstRef *out);

  b32 expect_token(TokenKind expected_kind, Token *out=nullptr);
  b32 next(Token *out);
  b32 peek(Token *out);
  AstRef add_node(AstNode node);

  b32 is_at_end() { return idx == tokens.len; }
};

void Parser::init(StringInterner *strings, Allocator ref_allocator, Vector<AstNode> *nodes, Slice<Token> tokens) {
  this->strings = strings;
  this->nodes = nodes;
  this->tokens = tokens;
  this->ref_allocator = ref_allocator;

  idx = 0;
}

AstRef Parser::add_node(AstNode node) {
  AstRef ref = nodes->len;
  nodes->push(node);
  return ref;
}

b32 Parser::next(Token *out) {
  if (idx == tokens.len) {
    return false;
  }

  *out = tokens[idx];
  idx += 1;

  return true;
}

b32 Parser::peek(Token *out) {
  if (idx == tokens.len) {
    return false;
  }

  *out = tokens[idx];

  return true;
}

b32 Parser::parse_module(AstRef *out) {
  Vector<AstRef> items;
  items.init(ref_allocator);

  while (!is_at_end()) {
    AstRef item;
    Try(parse_item(&item));
    items.push(item);
  }

  AstNode n;
  n.kind = Ast_module;
  n.module.items = items.move();

  AstRef mod = add_node(n);

  *out = mod;

  return true;
}

b32 Parser::parse_item(AstRef *out) {
  return parse_item_function(out);
}

b32 Parser::parse_item_function(AstRef *out) {
  Try(expect_token(Tok_keyword_fn));

  AstRef name_ref;
  Try(parse_identifier(&name_ref));

  Try(expect_token(Tok_paren_open));
  Try(expect_token(Tok_paren_close));

  Try(expect_token(Tok_colon));

  AstRef return_type_ref;
  Try(parse_type(&return_type_ref));

  Try(expect_token(Tok_brace_open));

  Vector<AstRef> scope;
  scope.init(ref_allocator);

  while (!is_at_end()) {
    Token tok;
    Try(peek(&tok));
    if (tok.kind == Tok_brace_close) {
      break;
    }

    AstRef statement;
    Try(parse_statement(&statement));
    scope.push(statement);
  }

  Try(expect_token(Tok_brace_close));

  AstNode body;
  body.kind = Ast_scope;
  body.scope.statements = scope.move();

  AstRef body_ref = add_node(body);

  AstNode node;
  node.kind = Ast_function;
  node.function.name = name_ref;
  node.function.return_type = return_type_ref;
  node.function.body = body_ref;

  AstRef function_ref = add_node(node);

  *out = function_ref;

  return true;
}

b32 Parser::parse_statement(AstRef *out) {
  Token tok;
  Try(peek(&tok));

  if (tok.kind == Tok_keyword_return) {
    return parse_statement_return(out);
  }

  AstRef identifier_ref;
  Try(parse_identifier(&identifier_ref));

  Try(peek(&tok));

  if (tok.kind == Tok_colon) {
    next(&tok);

    AstRef type_ref;
    Try(parse_type(&type_ref));

    Try(expect_token(Tok_equals));

    AstRef initial_value_ref;
    Try(parse_expression(&initial_value_ref));

    Try(expect_token(Tok_semicolon));

    AstNode n;
    n.kind = Ast_declaration;
    n.declaration.name = identifier_ref;
    n.declaration.type = type_ref;
    n.declaration.initial_value = initial_value_ref;

    *out = add_node(n);

    return true;
  }

  if (tok.kind == Tok_equals) {
    next(&tok);

    AstRef value_ref;
    Try(parse_expression(&value_ref));

    Try(expect_token(Tok_semicolon));

    AstNode n;
    n.kind = Ast_assign;
    n.assign.lhs = identifier_ref;
    n.assign.value = value_ref;

    *out = add_node(n);

    return true;
  }

  return false;
}

b32 Parser::parse_statement_return(AstRef *out) {
  Try(expect_token(Tok_keyword_return));

  AstRef value_ref;
  Try(parse_expression(&value_ref));
  Try(expect_token(Tok_semicolon));

  AstNode n;
  n.kind = Ast_return;
  n.ret.value = value_ref;

  *out = add_node(n);

  return true;
}

b32 Parser::parse_identifier(AstRef *out) {
  Token tok;
  Try(expect_token(Tok_identifier, &tok));

  StrKey key = strings->add(tok.str());

  AstNode identifier;
  identifier.span = tok.span;
  identifier.kind = Ast_identifier;
  identifier.identifier.key = key;

  *out = add_node(identifier);

  return true;
}

b32 Parser::parse_type(AstRef *out) {
  return parse_identifier(out);
}

b32 Parser::parse_literal_int(AstRef *out) {
  Token tok;
  Try(expect_token(Tok_literal_int, &tok));

  AstNode node;
  node.span = tok.span;
  node.kind = Ast_literal_int;

  *out = add_node(node);

  return true;
}

enum Precedence : u32 {
  Left,
  Right,
  Ambiguous,
};

Precedence determine_precedence() {
  return Ambiguous;
}

b32 Parser::parse_expression(AstRef *out) {
  return parse_literal_int(out);
}

b32 Parser::expect_token(TokenKind expected_kind, Token *out) {
  Token tok;
  Try(next(&tok));

  if (tok.kind != expected_kind) {
    return false;
  }

  if (out) {
    *out = tok;
  }

  return true;
}

b32 parse(ParseContext *ctx, Slice<Token> tokens, AstRef *root) {
  Parser parser;
  parser.init(ctx->strings, ctx->ref_allocator, ctx->nodes, tokens);
  return parser.parse_module(root);
}

