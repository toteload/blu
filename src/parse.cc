#include "blu.hh"

struct Parser {
  ParseContext ctx;

  Slice<Token> tokens;
  Token *at;

  void init(ParseContext ctx, Slice<Token> tokens);

  b32 parse_root(AstNode **out);
  b32 parse_module(AstNode **out);

  b32 parse_builtin(AstNode **out);

  b32 parse_item(AstNode **out);
  b32 parse_items(AstNode **out);

  b32 parse_parameter(AstNode **out);
  b32 parse_function(AstNode **out);

  b32 parse_type(AstNode **out);

  b32 parse_scope(AstNode **out);

  b32 parse_break(AstNode **out);
  b32 parse_continue(AstNode **out);
  b32 parse_return(AstNode **out);
  b32 parse_declaration(AstNode **out);
  b32 parse_literal_int(AstNode **out);
  b32 parse_literal_string(AstNode **out);
  b32 parse_literal_array_or_slice(AstNode **out);
  b32 parse_identifier(AstNode **out);

  b32 parse_while(AstNode **out);
  b32 parse_for(AstNode **out);

  b32 parse_arguments(AstNode **out);

  b32 parse_metadata(AstNode **out);

  b32 parse_expression(AstNode **out, BinaryOpKind prev_op = BinaryOpKind_max);
  b32 parse_base_expression(AstNode **out);
  b32 parse_base_expression_pre(AstNode **out);
  b32 parse_if_else(AstNode **out);
  b32 parse_call(AstNode **out);

  b32 expect_token(TokenKind expected_kind, Token *out = nullptr);
  b32 next(Token *out = nullptr);
  b32 peek(Token *out);
  b32 peek2(Token *out);

  u32 token_offset() { return cast<u32>(at - tokens.data); }

  AstNode *alloc_node() {
    AstNode *n          = ctx.nodes->alloc();
    n->token_span.start = token_offset();
    return n;
  }

  Token *skip_comments_from(Token *p) {
    while (p != tokens.end() && p->kind == Tok_line_comment) {
      p += 1;
    }

    return p;
  }

  b32 is_at_end() {
    at = skip_comments_from(at);
    return at == tokens.end();
  }
};

void Parser::init(ParseContext ctx, Slice<Token> tokens) {
  this->ctx    = ctx;
  this->tokens = tokens;
  at           = tokens.data;
}

b32 Parser::next(Token *out) {
  if (is_at_end()) {
    return false;
  }

  if (out != nullptr) {
    *out = *at;
  }

  at += 1;

  return true;
}

b32 Parser::peek(Token *out) {
  if (is_at_end()) {
    return false;
  }

  *out = *at;

  return true;
}

b32 Parser::peek2(Token *out) {
  Token *lookahead = skip_comments_from(at + 1);
  if (lookahead == tokens.end()) {
    return false;
  }

  *out = *lookahead;

  return true;
}

b32 Parser::parse_root(AstNode **out) {
  AstNode *n = alloc_node();

  AstNode *items = nullptr;
  Try(parse_items(&items));

  n->kind           = Ast_module;
  n->module.items   = items;
  n->token_span.end = token_offset();

  *out = n;

  return true;
}

b32 Parser::parse_module(AstNode **out) {
  AstNode *n = alloc_node();

  Token tok;
  Try(expect_token(Tok_keyword_module));

  Try(expect_token(Tok_brace_open));

  AstNode *items = nullptr;
  Try(parse_items(&items));

  Try(expect_token(Tok_brace_close));

  n->kind           = Ast_module;
  n->module.items   = items;
  n->token_span.end = token_offset();

  *out = n;

  return true;
}

b32 Parser::parse_builtin(AstNode **out) {
  Token tok;
  next(&tok);

  if (true /* is include builtin */) {
    AstNode *filename;
    Try(parse_literal_string(&filename));

    AstNode *n = alloc_node();

    n->kind           = Ast_builtin;
    n->builtin.kind   = Builtin_include;
    n->builtin.value  = filename;
    n->token_span.end = token_offset();

    *out = n;

    return true;
  }

  return false;
}

b32 Parser::parse_item(AstNode **out) {
  Token tok;
  Try(peek(&tok));

  if (tok.kind == Tok_builtin) {
    return parse_builtin(out);
  }

  return parse_declaration(out);
}

b32 Parser::parse_items(AstNode **out) {
  AstNode *items = nullptr;
  AstNode *last  = nullptr;

  while (!is_at_end()) {
    Token tok;
    Try(peek(&tok));

    if (tok.kind == Tok_brace_close) {
      break;
    }
    
    AstNode *item = nullptr;
    Try(parse_item(&item));

    if (!items) {
      items = item;
    } else {
      last->next = item;
    }

    last = item;
  }

  if (last) {
    last->next = nullptr;
  }

  *out = items;

  return true;
}

b32 Parser::parse_parameter(AstNode **out) {
  AstNode *n = alloc_node();

  AstNode *name;
  Try(parse_identifier(&name));

  Try(expect_token(Tok_colon));

  AstNode *type;
  Try(parse_type(&type));

  n->kind           = Ast_param;
  n->param.name     = name;
  n->param.type     = type;
  n->token_span.end = token_offset();

  *out = n;

  return true;
}

b32 Parser::parse_function(AstNode **out) {
  AstNode *n = alloc_node();

  Try(expect_token(Tok_keyword_fn));
  Try(expect_token(Tok_paren_open));

  AstNode *params = nullptr;
  AstNode *last   = nullptr;

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

    if (!params) {
      params = param;
    } else {
      last->next = param;
    }

    last = param;
  }

  if (last) {
    last->next = nullptr;
  }

  Try(expect_token(Tok_paren_close));

  AstNode *return_type;
  Try(parse_type(&return_type));

  AstNode *body;
  Try(parse_scope(&body));

  n->kind                 = Ast_function;
  n->function.params      = params;
  n->function.return_type = return_type;
  n->function.body        = body;
  n->token_span.end       = token_offset();

  *out = n;

  return true;
}

b32 Parser::parse_scope(AstNode **out) {
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

b32 Parser::parse_identifier(AstNode **out) {
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

b32 Parser::parse_literal_int(AstNode **out) {
  AstNode *n = alloc_node();

  Token tok;
  Try(expect_token(Tok_literal_int, &tok));

  n->kind           = Ast_literal_int;
  n->token_span.end = token_offset();

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

  n->kind                         = Ast_literal_array_or_slice;
  n->array_or_slice.declared_type = type;
  n->array_or_slice.items         = items;
  n->token_span.end               = token_offset();

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

  n->kind           = Ast_while;
  n->while_.cond    = cond;
  n->while_.body    = body;
  n->token_span.end = token_offset();

  *out = n;

  return true;
}

b32 Parser::parse_for(AstNode **out) {
  AstNode *n = alloc_node();

  Token tok;
  Try(expect_token(Tok_keyword_for, &tok));

  AstNode *item;
  Try(parse_identifier(&item));

  Try(expect_token(Tok_keyword_in));

  AstNode *iterable;
  Try(parse_expression(&iterable));

  AstNode *body;
  Try(parse_scope(&body));

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

b32 Parser::parse_base_expression(AstNode **out) {
  AstNode *pre = nullptr;

  Try(parse_base_expression_pre(&pre));

  while (!is_at_end()) {
    Token tok;
    if (!peek(&tok)) {
      break;
    }

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

      if (tok.kind == Tok_star) {
        next();

        n->kind = Ast_deref;
        n->deref.value = pre;
        n->token_span.end = token_offset();

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

  default: return false;
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
  Try(parse_scope(&then));

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
  Try(parse_scope(&otherwise));

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
