#include "blu.hh"

struct Parser {
  CompilerContext *ctx;

  usize idx;

  void init(CompilerContext *ctx);

  b32 parse_module(AstNode **out);

  b32 parse_top_level(AstNode **out);

  b32 parse_parameter(AstNode **out);
  b32 parse_function(AstNode **out);

  b32 parse_type(AstNode **out);

  b32 parse_scope(AstNode **out);

  b32 parse_statement(AstNode **out);
  b32 parse_break(AstNode **out);
  b32 parse_continue(AstNode **out);
  b32 parse_statement_return(AstNode **out);
  b32 parse_declaration(AstNode **out);
  b32 parse_literal_int(AstNode **out);
  b32 parse_identifier(AstNode **out);

  b32 parse_while(AstNode **out);

  b32 parse_expression(AstNode **out, BinaryOpKind prev_op = BinaryOpKind_max);
  b32 parse_base_expression(AstNode **out);
  b32 parse_if_else_expression(AstNode **out);
  b32 parse_call(AstNode **out);

  b32 expect_token(TokenKind expected_kind, Token *out = nullptr);
  b32 next(Token *out = nullptr);
  b32 peek(Token *out);
  b32 peek2(Token *out);

  AstNode *alloc_node() { return ctx->nodes.alloc(); }

  b32 is_at_end() { return idx == ctx->tokens.len; }

  void add_unexpected_token_message(Token token);
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

b32 Parser::parse_module(AstNode **out) {
  AstNode *n = alloc_node();

  AstNode *items = nullptr;
  AstNode *tail  = nullptr;

  while (!is_at_end()) {
    AstNode *item;
    Try(parse_top_level(&item));

    if (!items) {
      items = item;
    } else {
      tail->next = item;
    }

    tail = item;
  }

  n->kind         = Ast_module;
  n->module.items = items;

  *out = n;

  return true;
}

b32 Parser::parse_top_level(AstNode **out) { return parse_declaration(out); }

b32 Parser::parse_parameter(AstNode **out) {
  AstNode *n = alloc_node();

  AstNode *name;
  Try(parse_identifier(&name));

  Try(expect_token(Tok_colon));

  AstNode *type;
  Try(parse_type(&type));

  n->kind       = Ast_param;
  n->param.name = name;
  n->param.type = type;

  *out = n;

  return true;
}

b32 Parser::parse_function(AstNode **out) {
  AstNode *n = alloc_node();

  Try(expect_token(Tok_keyword_fn));
  Try(expect_token(Tok_paren_open));

  AstNode *params     = nullptr;
  AstNode *param_tail = nullptr;

  while (!is_at_end()) {
    Token tok;
    Try(peek(&tok));
    if (tok.kind == Tok_paren_close) {
      break;
    }

    AstNode *param;
    Try(parse_parameter(&param));

    if (!params) {
      params = param;
    } else {
      param_tail->next = param;
    }

    param_tail = param;

    Try(peek(&tok));
    if (tok.kind != Tok_comma) {
      break;
    }

    Try(next(&tok));
  }

  Try(expect_token(Tok_paren_close));

  Try(expect_token(Tok_colon));

  AstNode *return_type;
  Try(parse_type(&return_type));

  AstNode *body;
  Try(parse_scope(&body));

  n->kind                 = Ast_function;
  n->function.params      = params;
  n->function.return_type = return_type;
  n->function.body        = body;

  *out = n;

  return true;
}

b32 Parser::parse_scope(AstNode **out) {
  AstNode *n = alloc_node();

  Try(expect_token(Tok_brace_open));

  AstNode *statements = nullptr;
  AstNode *tail       = nullptr;

  while (!is_at_end()) {
    Token tok;
    Try(peek(&tok));
    if (tok.kind == Tok_brace_close) {
      break;
    }

    AstNode *x;
    Try(parse_statement(&x));

    if (!statements) {
      statements = x;
    } else {
      tail->next = x;
    }

    tail = x;
  }

  Try(expect_token(Tok_brace_close));

  n->kind             = Ast_scope;
  n->scope.statements = statements;

  *out = n;

  return true;
}

b32 Parser::parse_declaration(AstNode **out) {
  AstNode *n = alloc_node();

  AstNode *identifier;
  Try(parse_identifier(&identifier));

  Try(expect_token(Tok_colon));

  b32 is_const          = false;
  b32 has_explicit_type = true;

  Token tok;
  Try(peek(&tok));
  if (tok.kind == Tok_colon) {
    is_const          = true;
    has_explicit_type = false;
  } else if (tok.kind == Tok_equals) {
    has_explicit_type = false;
  }

  AstNode *type = nullptr;
  if (has_explicit_type) {
    Try(parse_type(&type));

    Try(next(&tok));

    if (tok.kind != Tok_equals && tok.kind != Tok_colon) {
      add_unexpected_token_message(tok);
      return false;
    }
  } else {
    next();
  }

  AstNode *value;
  Try(parse_expression(&value));

  Try(expect_token(Tok_semicolon));

  n->kind                 = Ast_declaration;
  n->declaration.is_const = is_const;
  n->declaration.name     = identifier;
  n->declaration.type     = type;
  n->declaration.value    = value;

  *out = n;

  return true;
}

b32 Parser::parse_break(AstNode **out) {
  AstNode *n = alloc_node();

  n->kind = Ast_break;

  Token tok;
  Try(expect_token(Tok_keyword_break, &tok));
  Try(expect_token(Tok_semicolon));

  *out = n;

  return true;
}

b32 Parser::parse_continue(AstNode **out) {
  AstNode *n = alloc_node();

  n->kind = Ast_continue;

  Token tok;
  Try(expect_token(Tok_keyword_continue, &tok));
  Try(expect_token(Tok_semicolon));

  *out = n;

  return true;
}

b32 Parser::parse_statement(AstNode **out) {
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

  AstNode *expr;
  Try(parse_expression(&expr));

  if (peek(&tok) && tok.kind == Tok_equals) {
    next();

    AstNode *n = alloc_node();

    AstNode *value;
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

b32 Parser::parse_statement_return(AstNode **out) {
  AstNode *n = alloc_node();

  Try(expect_token(Tok_keyword_return));

  AstNode *value_ref;
  Try(parse_expression(&value_ref));
  Try(expect_token(Tok_semicolon));

  n->kind          = Ast_return;
  n->_return.value = value_ref;

  *out = n;

  return true;
}

b32 Parser::parse_identifier(AstNode **out) {
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

b32 Parser::parse_type(AstNode **out) { return parse_identifier(out); }

b32 Parser::parse_literal_int(AstNode **out) {
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

b32 Parser::parse_while(AstNode **out) {
  AstNode *n = alloc_node();

  Token tok;
  Try(expect_token(Tok_keyword_while, &tok));

  AstNode *cond;
  Try(parse_expression(&cond));

  AstNode *body;
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

b32 Parser::parse_base_expression(AstNode **out) {
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

  if (tok.kind == Tok_keyword_fn) {
    return parse_function(out);
  }

  if (is_unary_op_token(tok.kind)) {
    next();

    AstNode *n = alloc_node();

    AstNode *value;
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

b32 Parser::parse_expression(AstNode **out, BinaryOpKind prev_op) {
  AstNode *lhs;
  Try(parse_base_expression(&lhs));

  // Parse function call(s)
  while (!is_at_end()) {
    Token tok;
    if (!peek(&tok) || tok.kind != Tok_paren_open) {
      break;
    }

    next();

    AstNode *n = alloc_node();

    Vector<AstNode *> arguments;
    arguments.init(ctx->ref_allocator);

    AstNode *args = nullptr;
    AstNode *tail = nullptr;

    while (!is_at_end()) {
      Token tok;
      Try(peek(&tok));

      if (tok.kind == Tok_paren_close) {
        break;
      }

      AstNode *arg;
      Try(parse_expression(&arg));

      if (!args) {
        args = arg;
      } else {
        tail->next = arg;
      }

      tail = arg;

      Try(peek(&tok));
      if (tok.kind != Tok_comma) {
        break;
      }

      next();
    }

    Try(expect_token(Tok_paren_close));

    n->kind      = Ast_call;
    n->call.f    = lhs;
    n->call.args = args;

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

    AstNode *rhs;
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

b32 Parser::parse_if_else_expression(AstNode **out) {
  AstNode *n = alloc_node();

  Try(expect_token(Tok_keyword_if));

  AstNode *cond;
  Try(parse_expression(&cond));

  AstNode *then;
  Try(parse_scope(&then));

  n->kind         = Ast_if_else;
  n->if_else.cond = cond;
  n->if_else.then = then;

  Token tok;
  Try(peek(&tok));
  if (tok.kind != Tok_keyword_else) {
    n->if_else.otherwise = nullptr;

    *out = n;

    return true;
  }

  Try(next(&tok));

  AstNode *otherwise;
  Try(parse_scope(&otherwise));

  n->if_else.otherwise = otherwise;

  *out = n;

  return true;
}

void Parser::add_unexpected_token_message(Token token) {
  Str msg = ctx->arena.push_format_string("Unexpected token encountered %d\n", token.kind);
  ctx->messages.push({token.span, Error, msg});
}

b32 Parser::expect_token(TokenKind expected_kind, Token *out) {
  Token tok;
  Try(next(&tok));

  if (tok.kind != expected_kind) {
    add_unexpected_token_message(tok);
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
