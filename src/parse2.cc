#include "blu.hh"

struct Parser {
  ParseContext *ctx;

  TokenIndex at;
  Tokens *tokens;

  void init(ParseContext *ctx, Tokens *tokens);

  // Parse functions

  b32 parse_root(NodeIndex *out);
  b32 parse_declaration(NodeIndex *out);

  b32 parse_literal_int(NodeIndex *out);
  b32 parse_literal_string(NodeIndex *out);

  b32 parse_block(NodeIndex *out);

  b32 parse_type(NodeIndex *out);

  b32 parse_function(NodeIndex *out);

  b32 parse_return(NodeIndex *out);

  // ---

//  b32 parse_module(NodeIndex *out);
//
//  b32 parse_builtin(NodeIndex *out);
//
//  b32 parse_item(NodeIndex *out);
//  b32 parse_items(NodeIndex *out);
//
//  b32 parse_parameter(NodeIndex *out);
//
//  b32 parse_break(NodeIndex *out);
//  b32 parse_continue(NodeIndex *out);
//  b32 parse_literal_array_or_slice(NodeIndex *out);
//  b32 parse_identifier(NodeIndex *out);
//  b32 parse_simple_identifier(NodeIndex *out);
//
//  b32 parse_while(NodeIndex *out);
//  b32 parse_for(NodeIndex *out);
//
//  b32 parse_arguments(NodeIndex *out);
//
//  b32 parse_metadata(NodeIndex *out);
//
//  b32 parse_expression(NodeIndex *out, BinaryOpKind prev_op = BinaryOpKind_max);
//  b32 parse_base_expression(NodeIndex *out);
//  b32 parse_base_expression_pre(NodeIndex *out);
//  b32 parse_if_else(NodeIndex *out);
//  b32 parse_call(NodeIndex *out);

  b32 expect_token(TokenKind expected_kind, TokenKind *out = nullptr);

  b32 next(TokenKind *out = nullptr);
  b32 peek(TokenKind *out);
  b32 peek2(TokenKind *out);

  TokenIndex current_token_index() { return at; }

  b32 is_token_index_end(TokenIndex idx) {
    return idx.idx == tokens->len();
  }

  NodeIndex alloc_node() {
    return ctx->nodes->alloc_node();
  }

  TokenIndex skip_comments_from(TokenIndex idx) {
    while (!is_token_index_end(idx) && tokens->kind(idx) == Tok_line_comment) {
      idx = { idx.idx + 1 };
    }

    return idx;
  }

  b32 is_at_end() {
    at = skip_comments_from(at);
    return is_token_index_end(at);
  }
};

void Parser::init(ParseContext *ctx, Tokens *tokens) {
  this->ctx    = ctx;
  this->tokens = tokens;
  at           = { 0 };
}

b32 Parser::next(TokenKind *out) {
  if (is_at_end()) {
    return false;
  }

  if (out != nullptr) {
    *out = tokens->kind(at);
  }

  at = { at.idx + 1 };

  return true;
}

b32 Parser::peek(TokenKind *out) {
  if (is_at_end()) {
    return false;
  }

  *out = tokens->kind(at);

  return true;
}

b32 Parser::peek2(TokenKind *out) {
  TokenIndex lookahead = skip_comments_from({ at.idx + 1 });
  if (is_token_index_end(lookahead)) {
    return false;
  }

  *out = tokens->kind(lookahead);

  return true;
}

b32 Parser::parse_root(NodeIndex *out) {
  NodeIndex n = alloc_node();

  SegmentList<Item> items;

  while (!is_at_end()) {
    TokenKind kind;
    Try(peek(&kind));

    if (kind == Tok_brace_close) {
      break;
    }

    NodeIndex item;
    Try(parse_item(&item));

    items.append(ctx->tmp_arena, item);
  }

  ctx->nodes->set_kind(n, Ast_root);

  // TODO save the items

  *out = n;

  return true;
}

b32 Parser::parse_function_type(NodeIndex *out) {
  NodeIndex n = alloc_node();

  TokenIndex expected_fn_token_index = at;

  Try(expect_token(Tok_keyword_fn));
  Try(expect_token(Tok_paren_open));

  SegmentList<Param> params;

  while (!is_at_end()) {
    Token tok;
    Try(peek(&tok));

    if (tok.kind == Tok_comma) {
      next(&tok);
    }

    if (tok.kind == Tok_paren_close) {
      break;
    }

    AstNode *param;
    Try(parse_parameter(&param));
  }

  Try(expect_token(Tok_paren_close));

  NodeIndex return_type;
  Try(parse_type(&return_type));

  return true;
}

b32 Parser::parse_function(AstNode **out) {
    NodeIndex body;
  Try(parse_block(&body));

  ctx->nodes->set_kind(n, Ast_function);
  ctx->nodes->set_data(n, {
    .node_and_node = {
      .a_idx = function_type,
      .b_idx = body,
    },
  });
  ctx->nodes->set_main_token(n, expected_fn_token_index);

  *out = n;

  return true;
}

b32 Parser::parse_block(AstNode **out) {
  AstNode *n = alloc_node();

  Try(expect_token(Tok_brace_open));

  AstNode *expressions = nullptr;
  AstNode *last        = nullptr;

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
      last->next = x;
    }

    last = x;
  }

  if (last) {
    last->next = nullptr;
  }

  Try(expect_token(Tok_brace_close));

  n->kind              = Ast_scope;
  n->scope.expressions = expressions;
  n->token_span.end    = token_offset();

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
      return false;
    }
  } else {
    next();
  }

  AstNode *value = nullptr;
  Try(parse_expression(&value));

  n->kind                 = Ast_declaration;
  n->declaration.is_const = is_const;
  n->declaration.name     = identifier;
  n->declaration.type     = type;
  n->declaration.value    = value;
  n->token_span.end       = token_offset();

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

  Token tok;
  Try(expect_token(Tok_keyword_continue, &tok));

  n->kind           = Ast_continue;
  n->token_span.end = token_offset();

  *out = n;

  return true;
}

b32 Parser::parse_return(AstNode **out) {
  AstNode *n = alloc_node();

  Try(expect_token(Tok_keyword_return));

  AstNode *value_ref;
  Try(parse_expression(&value_ref));

  n->kind           = Ast_return;
  n->return_.value  = value_ref;
  n->token_span.end = token_offset();

  *out = n;

  return true;
}

b32 Parser::parse_simple_identifier(AstNode **out) {
  AstNode *n = alloc_node();

  Token tok;
  Try(expect_token(Tok_identifier, &tok));

  StrKey key = ctx.strings->add(tok.str);

  n->kind           = Ast_identifier;
  n->identifier.key = key;
  n->token_span.end = token_offset();

  *out = n;

  return true;
}

b32 Parser::parse_identifier(AstNode **out) {
  AstNode *base = alloc_node();

  Token tok;
  Try(expect_token(Tok_identifier, &tok));

  StrKey key = ctx.strings->add(tok.str);

  base->kind           = Ast_identifier;
  base->identifier.key = key;
  base->token_span.end = token_offset();

  while (!is_at_end()) {
    peek(&tok);

    if (tok.kind != Tok_dot) {
      break;
    }

    next();

    AstNode *field;
    Try(parse_simple_identifier(&field));

    AstNode *n            = alloc_node();
    n->kind               = Ast_field_access;
    n->field_access.base  = base;
    n->field_access.field = field;

    base = n;
  }

  *out = base;

  return true;
}

b32 Parser::parse_type(AstNode **out) {
  AstNode *n = alloc_node();
  n->kind    = Ast_type;

  Token tok;
  Try(peek(&tok));

  u32 flags = 0;

  if (tok.kind == Tok_keyword_distinct) {
    flags |= Distinct;
    next();

    Try(peek(&tok));
  }

  n->ast_type.flags = flags;

  if (tok.kind == Tok_star) {
    next();

    AstNode *base;
    Try(parse_type(&base));

    n->ast_type.kind  = Ast_type_pointer;
    n->ast_type.base  = base;
    n->token_span.end = token_offset();

    *out = n;

    return true;
  }

  if (tok.kind == Tok_bracket_open) {
    next();
    Try(expect_token(Tok_bracket_close));

    AstNode *base;
    Try(parse_type(&base));

    n->ast_type.kind  = Ast_type_slice;
    n->ast_type.base  = base;
    n->token_span.end = token_offset();

    *out = n;

    return true;
  }

  AstNode *identifier;
  Try(parse_identifier(&identifier));

  n->ast_type.kind  = Ast_type_identifier;
  n->ast_type.base  = identifier;
  n->token_span.end = token_offset();

  *out = n;

  return true;
}

b32 Parser::parse_literal_string(AstNode **out) {
  AstNode *n = alloc_node();

  Token tok;
  Try(expect_token(Tok_literal_string, &tok));

  n->kind           = Ast_literal_string;
  n->token_span.end = token_offset();

  *out = n;

  return true;
}

b32 Parser::parse_literal_int(NodeIndex *out) {
  NodeIndex n = alloc_node();

  TokenKind kind;
  Try(expect_token(Tok_literal_int, &kind));

  set_ast_kind(n, Ast_literal_int);
  set_ast_main_token();

  *out = n;

  return true;
}

b32 Parser::parse_literal_array_or_slice(AstNode **out) {
  AstNode *n = alloc_node();

  // ! The dot is not part of this function

  Token tok;
  Try(peek(&tok));

  AstNode *type = nullptr;
  if (tok.kind != Tok_brace_open) {
    Try(parse_type(&type));
  }

  Try(expect_token(Tok_brace_open));

  AstNode *items = nullptr;
  AstNode *last  = nullptr;

  while (!is_at_end()) {
    Token tok;
    Try(peek(&tok));

    if (tok.kind == Tok_comma) {
      next(&tok);
    }

    if (tok.kind == Tok_brace_close) {
      break;
    }

    AstNode *x;
    Try(parse_expression(&x));

    if (!items) {
      items = x;
    } else {
      last->next = x;
    }

    last = x;
  }

  if (last) {
    last->next = nullptr;
  }

  Try(expect_token(Tok_brace_close));

  n->kind                = Ast_literal_tuple;
  n->tuple.declared_type = type;
  n->tuple.items         = items;
  n->token_span.end      = token_offset();

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
  Try(parse_block(&body));

  n->kind           = Ast_while;
  n->while_.cond    = cond;
  n->while_.body    = body;
  n->token_span.end = token_offset();

  *out = n;

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

  if (last) {
    last->next = nullptr;
  }

  Try(expect_token(Tok_paren_close));

  *out = args;

  return true;
}

b32 Parser::parse_for(AstNode **out) {
  AstNode *n = alloc_node();

  Token tok;
  Try(expect_token(Tok_keyword_for, &tok));

  AstNode *iterable;
  Try(parse_expression(&iterable));

  Try(expect_token(Tok_keyword_do));

  AstNode *item;
  Try(parse_identifier(&item));

  AstNode *body;
  Try(parse_block(&body));

  n->kind           = Ast_for;
  n->for_.item      = item;
  n->for_.iterable  = iterable;
  n->for_.body      = body;
  n->token_span.end = token_offset();

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

b32 Parser::parse_base_expression_pre(AstNode **out) {
  Token tok;
  Try(peek(&tok));

  AstNode *base = nullptr;

  switch (tok.kind) {
    // clang-format off
  case Tok_keyword_for:      Try(parse_for(&base));         break;
  case Tok_keyword_while:    Try(parse_while(&base));       break;
  case Tok_keyword_continue: Try(parse_continue(&base));    break;
  case Tok_keyword_break:    Try(parse_break(&base));       break;
  case Tok_keyword_return:   Try(parse_return(&base));      break;
  case Tok_keyword_if:       Try(parse_if_else(&base));     break;
  case Tok_keyword_module:   Try(parse_module(&base));      break;
  case Tok_keyword_fn:       Try(parse_function(&base));    break;
  case Tok_keyword_cast:     Todo();                        break;
  case Tok_literal_int:      Try(parse_literal_int(&base)); break;
    // clang-format on

  case Tok_dot: {
    next();

    Try(peek(&tok));

    if (tok.kind == Tok_paren_open) {
      next();
      Try(parse_expression(&base));
      Try(expect_token(Tok_paren_close));
    } else {
      Try(parse_literal_array_or_slice(&base));
    }
  } break;

  case Tok_identifier: {
    Token tok2;
    if (peek2(&tok2) && tok2.kind == Tok_colon) {
      Try(parse_declaration(&base));
    } else {
      Try(parse_identifier(&base));
    }
  } break;

  case Tok_keyword_distinct:
  case Tok_star:
  case Tok_bracket_open: {
    Try(parse_type(&base));
  } break;

  case Tok_exclamation:
  case Tok_minus:
  case Tok_ampersand: {
    next();

    AstNode *n = alloc_node();

    AstNode *value;
    Try(parse_expression(&value));

    UnaryOpKind op;

    switch (tok.kind) {
      // clang-format off
    case Tok_exclamation: op = Not;       break;
    case Tok_minus:       op = Negate;    break;
    case Tok_ampersand:   op = AddressOf; break;

    default: { Unreachable(); } break;
      // clang-format on
    }

    n->kind           = Ast_unary_op;
    n->unary_op.kind  = op;
    n->unary_op.value = value;
    n->token_span.end = token_offset();

    base = n;
  } break;

  default:
    return false;
  }

  *out = base;

  return true;
}

b32 Parser::parse_base_expression(AstNode **out) {
  AstNode *pre = nullptr;

  Try(parse_base_expression_pre(&pre));

  while (!is_at_end()) {
    Token tok;
    peek(&tok);

    if (tok.kind == Tok_paren_open) {
      AstNode *n = alloc_node();

      AstNode *args = nullptr;
      Try(parse_arguments(&args));

      n->kind           = Ast_call;
      n->call.callee    = pre;
      n->call.args      = args;
      n->token_span.end = token_offset();

      pre = n;

      continue;
    }

    if (tok.kind == Tok_dot) {
      AstNode *n = alloc_node();

      next();

      Try(peek(&tok));

      if (tok.kind == Tok_star) {
        next();

        n->kind           = Ast_deref;
        n->deref.value    = pre;
        n->token_span.end = token_offset();

        pre = n;

        continue;
      }

      if (tok.kind == Tok_identifier) {
        AstNode *field;
        Try(parse_identifier(&field));

        n->kind               = Ast_field_access;
        n->field_access.base  = pre;
        n->field_access.field = field;

        pre = n;

        continue;
      }

      Todo();
    }

    break;
  }

  *out = pre;

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
    case Tok_plus_equals: op = AddAssign;         break;

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

    if (op == Assign) {
      n->kind         = Ast_assign;
      n->assign.lhs   = lhs;
      n->assign.value = rhs;
    } else {
      n->kind           = Ast_binary_op;
      n->binary_op.kind = cast<BinaryOpKind>(op);
      n->binary_op.lhs  = lhs;
      n->binary_op.rhs  = rhs;
    }
    n->token_span.end = token_offset();

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
  Try(parse_block(&then));

  n->kind         = Ast_if_else;
  n->if_else.cond = cond;
  n->if_else.then = then;

  Token tok;
  Try(peek(&tok));
  if (tok.kind != Tok_keyword_else) {
    n->if_else.otherwise = nullptr;
    n->token_span.end    = token_offset();

    *out = n;

    return true;
  }

  Try(next(&tok));

  AstNode *otherwise;
  Try(parse_block(&otherwise));

  n->if_else.otherwise = otherwise;
  n->token_span.end    = token_offset();

  *out = n;

  return true;
}

b32 Parser::expect_token(TokenKind expected_kind, Token *out) {
  Token tok;
  Try(next(&tok));

  if (tok.kind != expected_kind) {
    ctx.messages->error(
      ctx.src_idx,
      tok.span,
      "Unexpected token encountered. Expected {tokenkind}, but got {tokenkind}.",
      expected_kind,
      tok.kind
    );
    return false;
  }

  if (out) {
    *out = tok;
  }

  return true;
}

b32 parse(ParseContext ctx, Slice<Token> tokens, AstNode **root) {
  Parser parser;
  parser.init(ctx, tokens);
  return parser.parse_root(root);
}
