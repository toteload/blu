#include "blu.hh"

struct Parser {
  CompilerContext *ctx;

  usize idx;

  void init(CompilerContext *ctx);

  b32 parse_module(AstRef *out);

  b32 parse_item(AstRef *out);
  b32 parse_parameter(AstRef *out);
  b32 parse_item_function(AstRef *out);

  b32 parse_type(AstRef *out);

  b32 parse_scope(AstRef *out);

  b32 parse_break(AstRef *out);
  b32 parse_continue(AstRef *out);
  b32 parse_statement(AstRef *out);
  b32 parse_statement_return(AstRef *out);
  b32 parse_declaration(AstRef *out);
  b32 parse_literal_int(AstRef *out);
  b32 parse_identifier(AstRef *out);

  b32 parse_while(AstRef *out);

  b32 parse_expression(AstRef *out, BinaryOpKind prev_op = BinaryOpKind_max);
  b32 parse_base_expression(AstRef *out);
  b32 parse_if_else_expression(AstRef *out);
  b32 parse_call(AstRef *out);

  b32 expect_token(TokenKind expected_kind, Token *out = nullptr);
  b32 next(Token *out = nullptr);
  b32 peek(Token *out);
  b32 peek2(Token *out);

  AstNode *alloc_node() { return ctx->nodes.alloc(); }

  b32 is_at_end() { return idx == ctx->tokens.len; }
};

void Parser::init(CompilerContext *ctx) {
  this->ctx = ctx;

  idx = 0;
}

b32 Parser::next(Token *out) {
  if (idx == ctx->tokens.len) {
    return false;
  }

  if (out != nullptr) {
    *out = ctx->tokens[idx];
  }

  idx += 1;

  return true;
}

b32 Parser::peek(Token *out) {
  if (idx == ctx->tokens.len) {
    return false;
  }

  *out = ctx->tokens[idx];

  return true;
}

b32 Parser::peek2(Token *out) {
  if (idx + 1 >= ctx->tokens.len) {
    return false;
  }

  *out = ctx->tokens[idx + 1];

  return true;
}

b32 Parser::parse_module(AstRef *out) {
  Vector<AstRef> items;
  items.init(ctx->ref_allocator);

  AstNode *n = alloc_node();

  while (!is_at_end()) {
    AstRef item;
    Try(parse_item(&item));
    items.push(item);
  }

  n->kind         = Ast_module;
  n->module.items = items.move();

  *out = n;

  return true;
}

b32 Parser::parse_item(AstRef *out) { return parse_item_function(out); }

b32 Parser::parse_parameter(AstRef *out) {
  AstNode *n = alloc_node();

  AstRef name;
  Try(parse_identifier(&name));

  Try(expect_token(Tok_colon));

  AstRef type;
  Try(parse_type(&type));

  n->kind       = Ast_param;
  n->param.name = name;
  n->param.type = type;

  *out = n;

  return true;
}

b32 Parser::parse_item_function(AstRef *out) {
  AstNode *n = alloc_node();

  AstRef name;
  Try(parse_identifier(&name));

  Try(expect_token(Tok_colon));
  Try(expect_token(Tok_colon));

  Try(expect_token(Tok_paren_open));

  Vector<AstRef> params;
  params.init(ctx->ref_allocator);

  while (!is_at_end()) {
    Token tok;
    Try(peek(&tok));
    if (tok.kind == Tok_paren_close) {
      break;
    }

    AstRef param;
    Try(parse_parameter(&param));

    params.push(param);

    Try(peek(&tok));
    if (tok.kind != Tok_comma) {
      break;
    }

    Try(next(&tok));
  }

  Try(expect_token(Tok_paren_close));

  Try(expect_token(Tok_colon));

  AstRef return_type;
  Try(parse_type(&return_type));

  AstRef body;
  Try(parse_scope(&body));

  n->kind                 = Ast_function;
  n->function.params      = params.move();
  n->function.name        = name;
  n->function.return_type = return_type;
  n->function.body        = body;

  *out = n;

  return true;
}

b32 Parser::parse_scope(AstRef *out) {
  AstNode *n = alloc_node();

  Vector<AstRef> scope;
  scope.init(ctx->ref_allocator);

  Try(expect_token(Tok_brace_open));

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

  n->kind             = Ast_scope;
  n->scope.statements = scope.move();

  *out = n;

  return true;
}

b32 Parser::parse_declaration(AstRef *out) {
  AstNode *n = alloc_node();

  AstRef identifier;
  Try(parse_identifier(&identifier));

  Try(expect_token(Tok_colon));

  AstRef type;
  Try(parse_type(&type));

  Try(expect_token(Tok_equals));

  AstRef initial_value;
  Try(parse_expression(&initial_value));

  Try(expect_token(Tok_semicolon));

  n->kind                      = Ast_declaration;
  n->declaration.name          = identifier;
  n->declaration.type          = type;
  n->declaration.initial_value = initial_value;

  *out = n;

  return true;
}

b32 Parser::parse_break(AstRef *out) {
  AstNode *n = alloc_node();

  n->kind = Ast_break;

  Token tok;
  Try(expect_token(Tok_keyword_break, &tok));
  Try(expect_token(Tok_semicolon));

  *out = n;

  return true;
}

b32 Parser::parse_continue(AstRef *out) {
  AstNode *n = alloc_node();

  n->kind = Ast_continue;

  Token tok;
  Try(expect_token(Tok_keyword_continue, &tok));
  Try(expect_token(Tok_semicolon));

  *out = n;

  return true;
}

b32 Parser::parse_statement(AstRef *out) {
  Token tok;
  Try(peek(&tok));

  if (tok.kind == Tok_keyword_break) {
    return parse_break(out);
  }

  if (tok.kind == Tok_keyword_continue) {
    return parse_continue(out);
  }

  if (tok.kind == Tok_keyword_while) {
    return parse_while(out);
  }

  if (tok.kind == Tok_keyword_return) {
    return parse_statement_return(out);
  }

  if (tok.kind == Tok_keyword_if) {
    return parse_if_else_expression(out);
  }

  if (tok.kind == Tok_identifier) {
    Token tok2;
    if (peek2(&tok2) && tok2.kind == Tok_colon) {
      return parse_declaration(out);
    }
  }

  AstRef expr;
  Try(parse_expression(&expr));

  if (peek(&tok) && tok.kind == Tok_equals) {
    next();

    AstNode *n = alloc_node();

    AstRef value;
    Try(parse_expression(&value));

    Try(expect_token(Tok_semicolon));

    n->kind         = Ast_assign;
    n->assign.lhs   = expr;
    n->assign.value = value;

    *out = n;

    return true;
  }

  Try(expect_token(Tok_semicolon));

  *out = expr;

  return true;
}

b32 Parser::parse_statement_return(AstRef *out) {
  AstNode *n = alloc_node();

  Try(expect_token(Tok_keyword_return));

  AstRef value_ref;
  Try(parse_expression(&value_ref));
  Try(expect_token(Tok_semicolon));

  n->kind      = Ast_return;
  n->ret.value = value_ref;

  *out = n;

  return true;
}

b32 Parser::parse_identifier(AstRef *out) {
  AstNode *n = alloc_node();

  Token tok;
  Try(expect_token(Tok_identifier, &tok));

  StrKey key = ctx->strings.add(tok.str());

  n->kind           = Ast_identifier;
  n->span           = tok.span;
  n->identifier.key = key;

  *out = n;

  return true;
}

b32 Parser::parse_type(AstRef *out) { return parse_identifier(out); }

b32 Parser::parse_literal_int(AstRef *out) {
  AstNode *n = alloc_node();

  Token tok;
  Try(expect_token(Tok_literal_int, &tok));

  n->kind = Ast_literal_int;
  n->span = tok.span;

  *out = n;

  return true;
}

enum Precedence : u32 {
  Left,
  Right,
};

b32 Parser::parse_while(AstRef *out) {
  AstNode *n = alloc_node();

  Token tok;
  Try(expect_token(Tok_keyword_while, &tok));

  AstRef cond;
  Try(parse_expression(&cond));

  AstRef body;
  Try(parse_scope(&body));

  n->kind        = Ast_while;
  n->_while.cond = cond;
  n->_while.body = body;

  *out = n;

  return true;
}

// clang-format off
static constexpr u8 binop_precedence_group[BinaryOpKind_max] = {
  0, 0, 0,
  1, 1,
  2, 2, 3,
  3, 3,
  4, 4, 4, 4, 4, 4,
  5, 5,
};
// clang-format on

Precedence determine_precedence(BinaryOpKind lhs, BinaryOpKind rhs) {
  u8 left_group  = binop_precedence_group[lhs];
  u8 right_group = binop_precedence_group[rhs];

  if (left_group == right_group || lhs < rhs) {
    return Left;
  }

  return Right;
}

b32 is_unary_op_token(TokenKind kind) { return kind == Tok_exclamation || kind == Tok_minus; }

b32 Parser::parse_base_expression(AstRef *out) {
  Token tok;
  Try(peek(&tok));

  if (tok.kind == Tok_paren_open) {
    next();

    Try(parse_base_expression(out));

    Try(expect_token(Tok_paren_close));

    return true;
  }

  if (tok.kind == Tok_keyword_if) {
    return parse_if_else_expression(out);
  }

  if (tok.kind == Tok_identifier) {
    return parse_identifier(out);
  }

  if (tok.kind == Tok_literal_int) {
    return parse_literal_int(out);
  }

  if (is_unary_op_token(tok.kind)) {
    next();

    AstNode *n = alloc_node();

    AstRef value;
    Try(parse_expression(&value));

    UnaryOpKind op;

    // clang-format off
    switch (tok.kind) {
    case Tok_exclamation: op = Not;    break;
    case Tok_minus:       op = Negate; break;
    default: { Unreachable(); } break;
    }
    // clang-format on

    n->kind           = Ast_unary_op;
    n->unary_op.kind  = op;
    n->unary_op.value = value;

    *out = n;

    return true;
  }

  return false;
}

b32 Parser::parse_expression(AstRef *out, BinaryOpKind prev_op) {
  AstRef lhs;
  Try(parse_base_expression(&lhs));

  // Parse function call(s)
  while (!is_at_end()) {
    Token tok;
    if (!peek(&tok) || tok.kind != Tok_paren_open) {
      break;
    }

    next();

    AstNode *n = alloc_node();

    Vector<AstRef> arguments;
    arguments.init(ctx->ref_allocator);

    while (!is_at_end()) {
      Token tok;
      Try(peek(&tok));

      if (tok.kind == Tok_paren_close) {
        break;
      }

      AstRef arg;
      Try(parse_expression(&arg));
      arguments.push(arg);

      Try(peek(&tok));
      if (tok.kind != Tok_comma) {
        break;
      }

      next();
    }

    Try(expect_token(Tok_paren_close));

    n->kind           = Ast_call;
    n->call.f         = lhs;
    n->call.arguments = arguments;

    lhs = n;
  }

  while (true) {
    Token tok;
    Try(peek(&tok));

    BinaryOpKind op = BinaryOpKind_max;

    // clang-format off
    switch (tok.kind) {
    case Tok_minus:       op = Sub; break;
    case Tok_plus:        op = Add; break;
    case Tok_star:        op = Mul; break;
    case Tok_slash:       op = Div; break;
    case Tok_percent:     op = Mod; break;
    case Tok_cmp_eq:      op = Cmp_equal; break;
    case Tok_cmp_ne:      op = Cmp_not_equal; break;
    case Tok_cmp_gt:      op = Cmp_greater_than; break;
    case Tok_cmp_ge:      op = Cmp_greater_equal; break;
    case Tok_cmp_lt:      op = Cmp_less_than; break;
    case Tok_cmp_le:      op = Cmp_less_equal; break;
    case Tok_keyword_and: op = Logical_and; break;
    case Tok_keyword_or:  op = Logical_or; break;
    case Tok_ampersand:   op = Bit_and; break;
    case Tok_bar:         op = Bit_or; break;
    case Tok_caret:       op = Bit_xor; break;
    case Tok_left_shift:  op = Bit_shift_left; break;
    case Tok_right_shift: op = Bit_shift_right; break;
    default: {
      *out = lhs;
      return true;
    }
    }
    // clang-format on

    Precedence precedence = Right;
    if (prev_op != BinaryOpKind_max) {
      precedence = determine_precedence(prev_op, op);
    }

    if (precedence == Left) {
      *out = lhs;
      return true;
    }

    next();

    AstNode *n = alloc_node();

    AstRef rhs;
    Try(parse_expression(&rhs, op));

    n->kind           = Ast_binary_op;
    n->binary_op.kind = op;
    n->binary_op.lhs  = lhs;
    n->binary_op.rhs  = rhs;

    lhs = n;
  }

  *out = lhs;

  return true;
}

b32 Parser::parse_if_else_expression(AstRef *out) {
  AstNode *n = alloc_node();

  Try(expect_token(Tok_keyword_if));

  AstRef cond;
  Try(parse_expression(&cond));

  AstRef then;
  Try(parse_scope(&then));

  n->kind         = Ast_if_else;
  n->if_else.cond = cond;
  n->if_else.then = then;

  Token tok;
  Try(peek(&tok));
  if (tok.kind != Tok_keyword_else) {
    n->if_else.otherwise = nil;

    *out = n;

    return true;
  }

  Try(next(&tok));

  AstRef otherwise;
  Try(parse_scope(&otherwise));

  n->if_else.otherwise = otherwise;

  *out = n;

  return true;
}

b32 Parser::expect_token(TokenKind expected_kind, Token *out) {
  Token tok;
  Try(next(&tok));

  if (tok.kind != expected_kind) {
    Str msg =
      ctx->arena.push_format_string("Expected token %d, but got %d\n", expected_kind, tok.kind);
    ctx->messages.push({tok.span, Error, msg});
    return false;
  }

  if (out) {
    *out = tok;
  }

  return true;
}

b32 parse(CompilerContext *ctx) {
  Parser parser;
  parser.init(ctx);

  return parser.parse_module(&ctx->root);
}
