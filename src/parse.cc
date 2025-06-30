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

  b32 parse_break(AstNode **out);
  b32 parse_continue(AstNode **out);
  b32 parse_return(AstNode **out);
  b32 parse_declaration(AstNode **out);
  b32 parse_literal_int(AstNode **out);
  b32 parse_identifier(AstNode **out);

  b32 parse_while(AstNode **out);

  b32 parse_arguments(AstNode **out);

  b32 parse_expression(AstNode **out, BinaryOpKind prev_op = BinaryOpKind_max);
  b32 parse_base_expression(AstNode **out);
  b32 parse_base_expression_pre(AstNode **out);
  b32 parse_base_expression_post(AstNode *base, AstNode **out);
  b32 parse_if_else(AstNode **out);
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

  AstNode *expressions = nullptr;
  AstNode *tail        = nullptr;

  while (!is_at_end()) {
    Token tok;
    Try(peek(&tok));
    if (tok.kind == Tok_brace_close) {
      break;
    }

    AstNode *x;
    Try(parse_expression(&x));

    if (!expressions) {
      expressions = x;
    } else {
      tail->next = x;
    }

    tail = x;
  }

  Try(expect_token(Tok_brace_close));

  n->kind              = Ast_scope;
  n->scope.expressions = expressions;

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

  *out = n;

  return true;
}

b32 Parser::parse_continue(AstNode **out) {
  AstNode *n = alloc_node();

  n->kind = Ast_continue;

  Token tok;
  Try(expect_token(Tok_keyword_continue, &tok));

  *out = n;

  return true;
}

b32 Parser::parse_return(AstNode **out) {
  AstNode *n = alloc_node();

  Try(expect_token(Tok_keyword_return));

  AstNode *value_ref;
  Try(parse_expression(&value_ref));

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

b32 Parser::parse_type(AstNode **out) {
  Token tok;
  Try(peek(&tok));

  if (tok.kind == Tok_star) {
    next();

    AstNode *n = alloc_node();

    AstNode *base;
    Try(parse_type(&base));

    n->kind         = Ast_type_pointer;
    n->span         = tok.span;
    n->pointer.base = base;

    *out = n;

    return true;
  }

  return parse_identifier(out);
}

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
  10, 10, 10,
  20, 20,
  30, 30,
  40, 40, 40,
  50, 50, 50, 50, 50, 50,
  60, 60,
  200,
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

b32 is_unary_op_token(TokenKind kind) {
  return kind == Tok_exclamation || kind == Tok_minus || kind == Tok_star;
}

b32 Parser::parse_base_expression(AstNode **out) {
  AstNode *pre = nullptr;

  Try(parse_base_expression_pre(&pre));
  Try(parse_base_expression_post(pre, out));

  return true;
}

b32 Parser::parse_arguments(AstNode **out) {
  AstNode *args = nullptr;
  AstNode *last = nullptr;

  Try(expect_token(Tok_paren_open));

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
      last->next = arg;
    }

    last = arg;

    Try(peek(&tok));
    if (tok.kind != Tok_comma) {
      break;
    }

    next();
  }

  Try(expect_token(Tok_paren_close));

  *out = args;

  return true;
}

b32 Parser::parse_base_expression_post(AstNode *base, AstNode **out) {
  AstNode *lhs = base;

  while (!is_at_end()) {
    Token tok;
    if (!peek(&tok)) {
      break;
    }

    if (tok.kind == Tok_paren_open) {
      AstNode *n = alloc_node();

      AstNode *args = nullptr;
      Try(parse_arguments(&args));

      n->kind        = Ast_call;
      n->call.callee = lhs;
      n->call.args   = args;

      lhs = n;

      continue;
    }

    if (tok.kind == Tok_dot) {
      next();

      AstNode *n = alloc_node();

      Try(expect_token(Tok_star));

      n->kind        = Ast_deref;
      n->deref.value = lhs;

      lhs = n;

      continue;
    }

    break;
  }

  *out = lhs;

  return true;
}

b32 Parser::parse_base_expression_pre(AstNode **out) {
  Token tok;
  Try(peek(&tok));

  AstNode *base = nullptr;

  if (tok.kind == Tok_paren_open) {
    next();

    Try(parse_expression(&base));

    Try(expect_token(Tok_paren_close));
  } else if (tok.kind == Tok_keyword_break) {
    Try(parse_break(&base));
  } else if (tok.kind == Tok_keyword_continue) {
    Try(parse_continue(&base));
  } else if (tok.kind == Tok_keyword_while) {
    Try(parse_while(&base));
  } else if (tok.kind == Tok_keyword_return) {
    Try(parse_return(&base));
  } else if (tok.kind == Tok_keyword_if) {
    Try(parse_if_else(&base));
  } else if (tok.kind == Tok_identifier) {
    Token tok2;
    if (peek2(&tok2) && tok2.kind == Tok_colon) {
      Try(parse_declaration(&base));
    } else {
      Try(parse_identifier(&base));
    }
  } else if (tok.kind == Tok_literal_int) {
    Try(parse_literal_int(&base));
  } else if (tok.kind == Tok_keyword_fn) {
    Try(parse_function(&base));
  } else if (is_unary_op_token(tok.kind)) {
    next();

    AstNode *n = alloc_node();

    AstNode *value;
    Try(parse_expression(&value));

    UnaryOpKind op;

    // clang-format off
    switch (tok.kind) {
    case Tok_exclamation: op = Not;       break;
    case Tok_minus:       op = Negate;    break;
    case Tok_star:        op = AddressOf; break;

    default: { Unreachable(); } break;
    }
    // clang-format on

    n->kind           = Ast_unary_op;
    n->unary_op.kind  = op;
    n->unary_op.value = value;

    base = n;
  }

  *out = base;

  return true;
}

b32 Parser::parse_expression(AstNode **out, BinaryOpKind prev_op) {
  AstNode *lhs;
  Try(parse_base_expression(&lhs));

  while (true) {
    Token tok;
    b32 has_peeked = peek(&tok);
    if (!has_peeked) {
      *out = lhs;
      return true;
    }

    BinaryOpKind op = BinaryOpKind_max;

    // clang-format off
    switch (tok.kind) {
    case Tok_minus:       op = Sub;               break;
    case Tok_plus:        op = Add;               break;
    case Tok_star:        op = Mul;               break;
    case Tok_slash:       op = Div;               break;
    case Tok_percent:     op = Mod;               break;
    case Tok_cmp_eq:      op = Cmp_equal;         break;
    case Tok_cmp_ne:      op = Cmp_not_equal;     break;
    case Tok_cmp_gt:      op = Cmp_greater_than;  break;
    case Tok_cmp_ge:      op = Cmp_greater_equal; break;
    case Tok_cmp_lt:      op = Cmp_less_than;     break;
    case Tok_cmp_le:      op = Cmp_less_equal;    break;
    case Tok_keyword_and: op = Logical_and;       break;
    case Tok_keyword_or:  op = Logical_or;        break;
    case Tok_ampersand:   op = Bit_and;           break;
    case Tok_bar:         op = Bit_or;            break;
    case Tok_caret:       op = Bit_xor;           break;
    case Tok_left_shift:  op = Bit_shift_left;    break;
    case Tok_right_shift: op = Bit_shift_right;   break;
    case Tok_equals:      op = Assign;            break;

    default: { *out = lhs; return true; }
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

    if (op < BinaryOpKind_max) {
      n->kind           = Ast_binary_op;
      n->binary_op.kind = cast<BinaryOpKind>(op);
      n->binary_op.lhs  = lhs;
      n->binary_op.rhs  = rhs;
    } else {
      Debug_assert(op == Assign);
      n->kind         = Ast_assign;
      n->assign.lhs   = lhs;
      n->assign.value = rhs;
    }

    lhs = n;
  }

  *out = lhs;

  return true;
}

b32 Parser::parse_if_else(AstNode **out) {
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
  Str s       = ctx->arena.push_format_string("Unexpected token encountered %d\n", token.kind);
  Message msg = {token.span, Error, s};
  Push_message(ctx->messages, msg);
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
