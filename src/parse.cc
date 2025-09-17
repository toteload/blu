#include "blu.hh"

static constexpr u32 Op_count = BinaryOpKind_max + AssignKind_max;

struct Parser {
  MessageManager *messages = nullptr;
  Tokens *tokens           = nullptr;
  Nodes *nodes             = nullptr;
  TokenIndex at            = {0};

  // -

  b32 parse_root(NodeIndex *out);
  b32 parse_block(NodeIndex *out);
  b32 parse_type(NodeIndex *out);
  b32 parse_declaration(NodeIndex *out);
  b32 parse_literal_int(NodeIndex *out);
  b32 parse_literal_sequence(NodeIndex *out);
  b32 parse_function(NodeIndex *out);
  b32 parse_while(NodeIndex *out);
  b32 parse_break(NodeIndex *out);
  b32 parse_continue(NodeIndex *out);
  b32 parse_return(NodeIndex *out);
  b32 parse_if_else(NodeIndex *out);
  b32 parse_base_expression(NodeIndex *out);
  b32 parse_expression(NodeIndex *out, u32 prev_op = Op_count);
  b32 parse_identifier(NodeIndex *out);

  // -

  b32 parse_parameter(Param *out);

  // - Helpers -

  b32 next(TokenKind *out = nullptr) {
    if (is_token_index_end(at)) {
      return false;
    }

    if (out != nullptr) {
      *out = tokens->kind(at);
    }

    at = advance(at);

    return true;
  }

  b32 expect_token(TokenKind expected_kind, TokenKind *out = nullptr) {
    TokenKind tok;
    Try(next(&tok));

    if (tok != expected_kind) {
      messages->error(
        "Unexpected token encountered. Expected {tokenkind}, but got {tokenkind}.",
        expected_kind,
        tok
      );

      return false;
    }

    if (out) {
      *out = tok;
    }

    return true;
  }

  b32 peek(TokenKind *out) {
    if (is_token_index_end(at)) {
      return false;
    }

    *out = tokens->kind(at);

    return true;
  }

  b32 peek2(TokenKind *out) {
    TokenIndex lookahead = advance(at);

    if (is_token_index_end(lookahead)) {
      return false;
    }

    *out = tokens->kind(lookahead);

    return true;
  }

  TokenIndex current_token_index() { return at; }

  b32 is_token_index_end(TokenIndex idx) { return idx.idx == tokens->end().idx; }

  TokenIndex advance(TokenIndex idx) {
    if (is_token_index_end(idx)) {
      return idx;
    }

    return {idx.idx + 1};
  }
};

b32 Parser::parse_root(NodeIndex *out) {
  NodeIndex ni = nodes->alloc();

  nodes->kind(ni)       = Ast_root;
  nodes->span(ni).start = at;

  nodes->data(ni).root.items.init();

  while (!is_token_index_end(at)) {
    auto decl = nodes->data(ni).root.items.push(nodes->segment_allocator);
    Try(parse_declaration(decl));
  }

  *out = ni;

  return true;
}

b32 Parser::parse_block(NodeIndex *out) {
  NodeIndex ni = nodes->alloc();

  nodes->kind(ni)       = Ast_block;
  nodes->span(ni).start = at;

  nodes->data(ni).block.items.init();

  Try(expect_token(Tok_brace_open));

  while (!is_token_index_end(at)) {
    TokenKind tok;
    Try(peek(&tok));
    if (tok == Tok_brace_close) {
      break;
    }

    auto e = nodes->data(ni).block.items.push(nodes->segment_allocator);
    Try(parse_expression(e));
  }

  Try(expect_token(Tok_brace_close));

  nodes->span(ni).end = at;

  *out = ni;

  return true;
}

b32 Parser::parse_type(NodeIndex *out) {
  NodeIndex ni = nodes->alloc();

  nodes->kind(ni)       = Ast_type;
  nodes->span(ni).start = at;

  TokenKind tok;
  Try(peek(&tok));

  u32 flags = 0;

  if (tok == Tok_keyword_distinct) {
    flags |= Distinct;
    next();

    Try(peek(&tok));
  }

  nodes->data(ni).type.flags = flags;

  if (tok == Tok_bracket_open) {
    next();
    Try(expect_token(Tok_bracket_close));

    NodeIndex base;
    Try(parse_type(&base));

    nodes->data(ni).type.kind      = Ast_type_slice;
    nodes->data(ni).type.data.base = base;
    nodes->span(ni).end            = at;

    *out = ni;

    return true;
  }

  if (tok == Tok_keyword_fn) {
    next();
    Try(expect_token(Tok_paren_open));

    nodes->data(ni).type.data.function.params.init();

    while (!is_token_index_end(at)) {
      Try(peek(&tok));

      if (tok == Tok_paren_close) {
        break;
      }

      auto param = nodes->data(ni).type.data.function.params.push(nodes->segment_allocator);
      Try(parse_type(param));

      Try(peek(&tok));
      if (tok != Tok_comma) {
        break;
      }

      next();
    }

    Try(expect_token(Tok_paren_close));

    NodeIndex return_type;
    Try(parse_type(&return_type));

    nodes->data(ni).type.kind                      = Ast_type_function;
    nodes->data(ni).type.data.function.return_type = return_type;
    nodes->span(ni).end                            = at;

    *out = ni;

    return true;
  }

  TokenIndex token_index = at;
  Try(expect_token(Tok_identifier));

  nodes->data(ni).type.kind             = Ast_type_identifier;
  nodes->data(ni).type.data.token_index = token_index;
  nodes->span(ni).end                   = at;

  *out = ni;

  return true;
}

b32 Parser::parse_declaration(NodeIndex *out) {
  NodeIndex ni = nodes->alloc();

  nodes->kind(ni)       = Ast_declaration;
  nodes->span(ni).start = at;

  TokenIndex name = at;
  Try(expect_token(Tok_identifier));

  Try(expect_token(Tok_colon));

  NodeIndex type;
  Try(parse_type(&type));

  Try(expect_token(Tok_equals));

  NodeIndex value;
  Try(parse_expression(&value));

  nodes->data(ni).declaration.name  = name;
  nodes->data(ni).declaration.type  = type;
  nodes->data(ni).declaration.value = value;
  nodes->span(ni).end               = at;

  *out = ni;

  return true;
}

b32 Parser::parse_literal_int(NodeIndex *out) {
  NodeIndex ni = nodes->alloc();

  nodes->kind(ni)       = Ast_literal_int;
  nodes->span(ni).start = at;

  Try(expect_token(Tok_literal_int));

  nodes->span(ni).end = at;

  *out = ni;

  return true;
}

b32 Parser::parse_literal_sequence(NodeIndex *out) {
  NodeIndex ni = nodes->alloc();

  nodes->kind(ni)       = Ast_literal_sequence;
  nodes->span(ni).start = at;

  nodes->data(ni).literal_sequence.items.init();

  Try(expect_token(Tok_dot));
  Try(expect_token(Tok_brace_open));

  while (!is_token_index_end(at)) {
    TokenKind tok;
    Try(peek(&tok));

    if (tok == Tok_comma) {
      next(&tok);
    }

    if (tok == Tok_brace_close) {
      break;
    }

    auto e = nodes->data(ni).literal_sequence.items.push(nodes->segment_allocator);
    Try(parse_expression(e));
  }

  Try(expect_token(Tok_brace_close));

  nodes->span(ni).end = at;

  *out = ni;

  return true;
}

b32 Parser::parse_function(NodeIndex *out) {
  NodeIndex ni = nodes->alloc();

  nodes->kind(ni)       = Ast_function;
  nodes->span(ni).start = at;

  nodes->data(ni).function.params.init();

  Try(expect_token(Tok_keyword_fn));
  Try(expect_token(Tok_paren_open));

  while (!is_token_index_end(at)) {
    TokenKind tok;
    Try(peek(&tok));

    if (tok == Tok_comma) {
      next(&tok);
    }

    if (tok == Tok_paren_close) {
      break;
    }

    auto param = nodes->data(ni).function.params.push(nodes->segment_allocator);
    Try(parse_parameter(param));
  }

  Try(expect_token(Tok_paren_close));

  NodeIndex return_type;
  Try(parse_type(&return_type));

  NodeIndex body;
  Try(parse_block(&body));

  nodes->data(ni).function.return_type = return_type;
  nodes->data(ni).function.body        = body;
  nodes->span(ni).end                  = at;

  *out = ni;

  return true;
}

b32 Parser::parse_while(NodeIndex *out) {
  NodeIndex ni = nodes->alloc();

  nodes->kind(ni)       = Ast_while;
  nodes->span(ni).start = at;

  TokenKind tok;
  Try(expect_token(Tok_keyword_while, &tok));

  NodeIndex cond;
  Try(parse_expression(&cond));

  NodeIndex body;
  Try(parse_block(&body));

  nodes->data(ni).while_.cond = cond;
  nodes->data(ni).while_.body = body;

  nodes->span(ni).end = at;

  *out = ni;

  return true;
}

b32 Parser::parse_break(NodeIndex *out) {
  NodeIndex ni = nodes->alloc();

  nodes->kind(ni)       = Ast_break;
  nodes->span(ni).start = at;

  Try(expect_token(Tok_keyword_break));

  nodes->span(ni).end = at;

  *out = ni;

  return true;
}

b32 Parser::parse_continue(NodeIndex *out) {
  NodeIndex ni = nodes->alloc();

  nodes->kind(ni)       = Ast_continue;
  nodes->span(ni).start = at;

  Try(expect_token(Tok_keyword_continue));

  nodes->span(ni).end = at;

  *out = ni;

  return true;
}

b32 Parser::parse_return(NodeIndex *out) {
  NodeIndex ni = nodes->alloc();

  nodes->kind(ni)       = Ast_return;
  nodes->span(ni).start = at;

  Try(expect_token(Tok_keyword_return));

  NodeIndex value;
  Try(parse_expression(&value));

  nodes->data(ni).return_.value = value;
  nodes->span(ni).end           = at;

  *out = ni;

  return true;
}

b32 Parser::parse_if_else(NodeIndex *out) {
  Try(expect_token(Tok_keyword_if));

  NodeIndex ni = nodes->alloc();

  nodes->kind(ni)       = Ast_if_else;
  nodes->span(ni).start = at;

  NodeIndex cond;
  Try(parse_expression(&cond));

  nodes->data(ni).if_else.cond = cond;

  NodeIndex then;
  Try(parse_block(&then));

  nodes->data(ni).if_else.then = then;

  TokenKind tok;
  Try(peek(&tok));
  if (tok != Tok_keyword_else) {
    nodes->data(ni).if_else.otherwise = OptionalNodeIndex_none;
    nodes->span(ni).end               = at;

    *out = ni;

    return true;
  }

  next();

  NodeIndex otherwise;
  Try(parse_block(&otherwise));

  nodes->data(ni).if_else.otherwise = OptionalNodeIndex::from_node_index(otherwise);
  nodes->span(ni).end               = at;

  *out = ni;

  return true;
}

b32 Parser::parse_base_expression(NodeIndex *out) {
  TokenKind tok;
  Try(peek(&tok));

  NodeIndex base;
  switch (tok) {
    // clang-format off
  case Tok_keyword_while:    Try(parse_while(&base));       break;
  case Tok_keyword_continue: Try(parse_continue(&base));    break;
  case Tok_keyword_break:    Try(parse_break(&base));       break;
  case Tok_keyword_return:   Try(parse_return(&base));      break;
  case Tok_keyword_if:       Try(parse_if_else(&base));     break;
  case Tok_keyword_fn:       Try(parse_function(&base));    break;
  case Tok_literal_int:      Try(parse_literal_int(&base)); break;
    // clang-format on

  case Tok_dot: {
    Try(parse_literal_sequence(&base));
  } break;

  case Tok_identifier: {
    TokenKind tok2;
    if (peek2(&tok2) && tok2 == Tok_colon) {
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
  case Tok_minus: {
    next();

    NodeIndex ni = nodes->alloc();

    nodes->kind(ni)       = Ast_unary_op;
    nodes->span(ni).start = at;

    UnaryOpKind op;
    switch (tok) {
      // clang-format off
    case Tok_exclamation: op = Not;       break;
    case Tok_minus:       op = Negate;    break;

    default: { Unreachable(); } break;
      // clang-format on
    }

    nodes->data(ni).unary_op.kind = op;

    NodeIndex value;
    Try(parse_expression(&value));

    nodes->data(ni).unary_op.value = value;
    nodes->span(ni).end            = at;

    base = ni;
  } break;

  default:
    return false;
  }

  while (!is_token_index_end(at)) {
    TokenKind tok;
    peek(&tok);

    if (tok == Tok_paren_open) {
      next();

      NodeIndex ni = nodes->alloc();

      nodes->kind(ni)             = Ast_call;
      nodes->span(ni).start       = at;
      nodes->data(ni).call.callee = base;

      nodes->data(ni).call.args.init();

      while (!is_token_index_end(at)) {
        Try(peek(&tok));

        if (tok == Tok_paren_close) {
          break;
        }

        auto arg = nodes->data(ni).call.args.push(nodes->segment_allocator);
        Try(parse_expression(arg));

        Try(peek(&tok));
        if (tok != Tok_comma) {
          break;
        }

        next();
      }

      Try(expect_token(Tok_paren_close));

      nodes->span(ni).end = at;

      base = ni;

      continue;
    }

    break;
  }

  *out = base;

  return true;
}

// clang-format off
static constexpr u8 op_precedence_group[Op_count] = {
  10, 10, 10,
  20, 20,
  30, 30,
  40, 40, 40,
  50, 50, 50, 50, 50, 50,
  60, 60,
  200,
};
// clang-format on

enum Precedence : u32 {
  Left,
  Right,
};

Precedence determine_precedence(u32 lhs, u32 rhs) {
  u8 left_group  = op_precedence_group[lhs];
  u8 right_group = op_precedence_group[rhs];

  if (left_group <= right_group) {
    return Left;
  }

  return Right;
}

b32 Parser::parse_expression(NodeIndex *out, u32 prev_op) {
  TokenIndex start = at;

  NodeIndex lhs;
  Try(parse_base_expression(&lhs));

  while (true) {
    TokenKind tok;
    b32 has_peeked = peek(&tok);
    if (!has_peeked) {
      *out = lhs;
      return true;
    }

    u32 op = Op_count;

    // clang-format off
    switch (tok) {
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

    case Tok_equals:      op = BinaryOpKind_max + Assign_normal; break;

    default: { *out = lhs; return true; }
    }
    // clang-format on

    Precedence precedence = Right;
    if (prev_op != Op_count) {
      precedence = determine_precedence(prev_op, op);
    }

    if (precedence == Left) {
      *out = lhs;
      return true;
    }

    next();

    NodeIndex rhs;
    Try(parse_expression(&rhs, op));

    NodeIndex ni = nodes->alloc();

    nodes->span(ni).start = start;

    if (op >= BinaryOpKind_max) {
      nodes->kind(ni)              = Ast_assign;
      nodes->data(ni).assign.kind  = Assign_normal;
      nodes->data(ni).assign.lhs   = lhs;
      nodes->data(ni).assign.value = rhs;
    } else {
      nodes->kind(ni)                = Ast_binary_op;
      nodes->data(ni).binary_op.kind = cast<BinaryOpKind>(op);
      nodes->data(ni).binary_op.lhs  = lhs;
      nodes->data(ni).binary_op.rhs  = rhs;
    }

    nodes->span(ni).end = at;

    lhs = ni;
  }

  *out = lhs;

  return true;
}

b32 Parser::parse_identifier(NodeIndex *out) {
  NodeIndex ni = nodes->alloc();

  nodes->kind(ni)       = Ast_identifier;
  nodes->span(ni).start = at;

  TokenIndex ti = at;
  Try(expect_token(Tok_identifier));

  nodes->data(ni).identifier.token_index = ti;
  nodes->span(ni).end                    = at;

  *out = ni;

  return true;
}

// b32 Parser::parse_identifier(NodeIndex *out) {
//   NodeIndex base = nodes->alloc();
//
//   Token tok;
//   Try(expect_token(Tok_identifier, &tok));
//
//   StrKey key = ctx.strings->add(tok.str);
//
//   base->kind           = Ast_identifier;
//   base->identifier.key = key;
//   base->token_span.end = token_offset();
//
//   while (!is_token_index_end(at)) {
//     peek(&tok);
//
//     if (tok.kind != Tok_dot) {
//       break;
//     }
//
//     next();
//
//     NodeIndex field;
//     Try(parse_simple_identifier(&field));
//
//     NodeIndex n            = alloc_node();
//     nodes->kind               = Ast_field_access;
//     nodes->field_access.base  = base;
//     nodes->field_access.field = field;
//
//     base = n;
//   }
//
//   *out = base;
//
//   return true;
// }

b32 Parser::parse_parameter(Param *out) {
  TokenIndex name = at;
  Try(expect_token(Tok_identifier));

  Try(expect_token(Tok_colon));

  NodeIndex type;
  Try(parse_type(&type));

  *out = Param{
    name,
    type,
  };

  return true;
}

b32 parse(ParseContext *ctx, Nodes *nodes) {
  Parser parser;
  parser.messages = ctx->messages;
  parser.tokens   = ctx->tokens;
  parser.nodes    = nodes;

  NodeIndex root;
  return parser.parse_root(&root);
}
