#include "blu.hh"

static constexpr u32 Op_count = BinaryOpKind_max + AssignKind_max;

struct Parser {
  MessageManager *messages = nullptr;
  Tokens *tokens           = nullptr;
  AstNodes *nodes          = nullptr;
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

  b32 parse_parameter(AstParam *out);

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
  auto node_index = nodes->alloc();
  auto start      = at;

  AstRoot root;
  root.items.init();

  while (!is_token_index_end(at)) {
    auto decl = root.items.push(nodes->segment_allocator);
    Try(parse_declaration(decl));
  }

  nodes->set(
    node_index,
    {
      Ast_root,
      {start, at},
      {.root = root},
    }
  );

  *out = node_index;

  return true;
}

b32 Parser::parse_block(NodeIndex *out) {
  auto node_index = nodes->alloc();
  auto start      = at;

  AstBlock block;
  block.items.init();

  Try(expect_token(Tok_brace_open));

  while (!is_token_index_end(at)) {
    TokenKind tok;
    Try(peek(&tok));
    if (tok == Tok_brace_close) {
      break;
    }

    auto e = block.items.push(nodes->segment_allocator);
    Try(parse_expression(e));
  }

  Try(expect_token(Tok_brace_close));

  nodes->set(
    node_index,
    {
      Ast_block,
      {start, at},
      {.block = block},
    }
  );

  *out = node_index;

  return true;
}

b32 Parser::parse_type(NodeIndex *out) {
  auto node_index = nodes->alloc();
  auto start      = at;

  TokenKind tok;
  Try(peek(&tok));

  switch (tok) {
  case Tok_bracket_open: {
    next();
    Try(expect_token(Tok_bracket_close));

    AstTypeSlice type_slice;
    Try(parse_type(&type_slice.base));

    nodes->set(
      node_index,
      {
        Ast_type_slice,
        {start, at},
        {.type_slice = type_slice},
      }
    );
  } break;
  case Tok_keyword_fn: {
    next();
    Try(expect_token(Tok_paren_open));

    AstTypeFunction type_function;
    type_function.params.init();

    while (!is_token_index_end(at)) {
      Try(peek(&tok));

      if (tok == Tok_paren_close) {
        break;
      }

      auto param = type_function.params.push(nodes->segment_allocator);
      Try(parse_type(param));

      Try(peek(&tok));
      if (tok != Tok_comma) {
        break;
      }

      next();
    }

    Try(expect_token(Tok_paren_close));

    Try(parse_type(&type_function.return_type));

    nodes->set(
      node_index,
      {
        Ast_type_function,
        {start, at},
        {.type_function = type_function},
      }
    );
  } break;
  case Tok_identifier: {
    AstAtom identifier;
    identifier.token_index = at;

    next();

    nodes->set(
      node_index,
      {
        Ast_identifier,
        {start, at},
        {.identifier = identifier},
      }
    );
  } break;
  default:
    return false;
  }

  *out = node_index;

  return true;
}

b32 Parser::parse_declaration(NodeIndex *out) {
  auto node_index = nodes->alloc();
  auto start      = at;

  AstDeclaration declaration;

  declaration.name = at;
  Try(expect_token(Tok_identifier));

  Try(expect_token(Tok_colon));

  Try(parse_type(&declaration.type));

  Try(expect_token(Tok_equals));

  Try(parse_expression(&declaration.value));

  nodes->set(
    node_index,
    {
      Ast_declaration,
      {start, at},
      {.declaration = declaration},
    }
  );

  *out = node_index;

  return true;
}

b32 Parser::parse_literal_int(NodeIndex *out) {
  auto node_index = nodes->alloc();
  auto start      = at;

  AstAtom literal_int;
  literal_int.token_index = at;

  Try(expect_token(Tok_literal_int));

  nodes->set(
    node_index,
    {
      Ast_literal_int,
      {start, at},
      {.literal_int = literal_int},
    }
  );

  *out = node_index;

  return true;
}

b32 Parser::parse_literal_sequence(NodeIndex *out) {
  auto node_index = nodes->alloc();
  auto start      = at;

  AstLiteralSequence literal_sequence;
  literal_sequence.items.init();

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

    auto e = literal_sequence.items.push(nodes->segment_allocator);
    Try(parse_expression(e));
  }

  Try(expect_token(Tok_brace_close));

  nodes->set(
    node_index,
    {
      Ast_literal_sequence,
      {start, at},
      {.literal_sequence = literal_sequence},
    }
  );

  *out = node_index;

  return true;
}

b32 Parser::parse_function(NodeIndex *out) {
  auto node_index = nodes->alloc();
  auto start      = at;

  AstFunction function;
  function.params.init();

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

    auto param = function.params.push(nodes->segment_allocator);
    Try(parse_parameter(param));
  }

  Try(expect_token(Tok_paren_close));

  Try(parse_type(&function.return_type));

  Try(parse_block(&function.body));

  nodes->set(
    node_index,
    {
      Ast_function,
      {start, at},
      {.function = function},
    }
  );

  *out = node_index;

  return true;
}

b32 Parser::parse_while(NodeIndex *out) {
  auto node_index = nodes->alloc();
  auto start      = at;

  TokenKind tok;
  Try(expect_token(Tok_keyword_while, &tok));

  AstWhile while_;

  Try(parse_expression(&while_.cond));

  Try(parse_block(&while_.body));

  nodes->set(
    node_index,
    {
      Ast_while,
      {start, at},
      {.while_ = while_},
    }
  );

  *out = node_index;

  return true;
}

b32 Parser::parse_break(NodeIndex *out) {
  auto start = at;

  Try(expect_token(Tok_keyword_break));

  *out = nodes->add({
    Ast_break,
    {start, at},
    {},
  });

  return true;
}

b32 Parser::parse_continue(NodeIndex *out) {
  auto start = at;

  Try(expect_token(Tok_keyword_continue));

  *out = nodes->add({
    Ast_continue,
    {start, at},
    {},
  });

  return true;
}

b32 Parser::parse_return(NodeIndex *out) {
  auto node_index = nodes->alloc();
  auto start      = at;

  Try(expect_token(Tok_keyword_return));

  AstReturn return_;

  Try(parse_expression(&return_.value));

  nodes->set(
    node_index,
    {
      Ast_return,
      {start, at},
      {.return_ = return_},
    }
  );

  *out = node_index;

  return true;
}

b32 Parser::parse_if_else(NodeIndex *out) {
  auto node_index = nodes->alloc();
  auto start      = at;

  Try(expect_token(Tok_keyword_if));

  AstIfElse if_else;

  Try(parse_expression(&if_else.cond));

  Try(parse_block(&if_else.then));

  TokenKind tok;
  Try(peek(&tok));
  if (tok != Tok_keyword_else) {
    if_else.otherwise = OptionalNodeIndex::none();

    nodes->set(
      node_index,
      {
        Ast_if_else,
        {start, at},
        {.if_else = if_else},
      }
    );

    *out = node_index;

    return true;
  }

  next();

  NodeIndex otherwise;
  Try(parse_block(&otherwise));

  if_else.otherwise = OptionalNodeIndex::from_index(otherwise);

  nodes->set(
    node_index,
    {
      Ast_if_else,
      {start, at},
      {.if_else = if_else},
    }
  );

  *out = node_index;

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
    auto node_index = nodes->alloc();
    auto start      = at;

    next();

    AstUnaryOp unary_op;

    switch (tok) {
      // clang-format off
    case Tok_exclamation: unary_op.kind = Not;    break;
    case Tok_minus:       unary_op.kind = Negate; break;

    default: { Unreachable(); } break;
      // clang-format on
    }

    Try(parse_expression(&unary_op.value));

    nodes->set(
      node_index,
      {
        Ast_unary_op,
        {start, at},
        {.unary_op = unary_op},
      }
    );

    base = node_index;
  } break;

  default:
    return false;
  }

  while (!is_token_index_end(at)) {
    TokenKind tok;
    peek(&tok);

    if (tok == Tok_paren_open) {
      auto node_index = nodes->alloc();
      auto start      = at;

      next();

      AstCall call;
      call.args.init();
      call.callee = base;

      while (!is_token_index_end(at)) {
        Try(peek(&tok));

        if (tok == Tok_paren_close) {
          break;
        }

        auto arg = call.args.push(nodes->segment_allocator);
        Try(parse_expression(arg));

        Try(peek(&tok));
        if (tok != Tok_comma) {
          break;
        }

        next();
      }

      Try(expect_token(Tok_paren_close));

      nodes->set(
        node_index,
        {
          Ast_call,
          {start, at},
          {.call = call},
        }
      );

      base = node_index;

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

    auto node_index = nodes->alloc();
    auto start      = at;

    next();

    NodeIndex rhs;
    Try(parse_expression(&rhs, op));

    if (op >= BinaryOpKind_max) {
      AstAssign assign;
      assign.kind  = Assign_normal;
      assign.lhs   = lhs;
      assign.value = rhs;

      nodes->set(
        node_index,
        {
          Ast_assign,
          {start, at},
          {.assign = assign},
        }
      );
    } else {
      AstBinaryOp binary_op;
      binary_op.kind = cast<BinaryOpKind>(op);
      binary_op.lhs  = lhs;
      binary_op.rhs  = rhs;

      nodes->set(
        node_index,
        {
          Ast_binary_op,
          {start, at},
          {.binary_op = binary_op},
        }
      );
    }

    lhs = node_index;
  }

  *out = lhs;

  return true;
}

b32 Parser::parse_identifier(NodeIndex *out) {
  auto start = at;

  AstAtom identifier;
  identifier.token_index = at;

  Try(expect_token(Tok_identifier));

  *out = nodes->add({
    Ast_identifier,
    {start, at},
    {.identifier = identifier},
  });

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

b32 Parser::parse_parameter(AstParam *out) {
  TokenIndex name = at;
  Try(expect_token(Tok_identifier));

  Try(expect_token(Tok_colon));

  NodeIndex type;
  Try(parse_type(&type));

  *out = AstParam{
    name,
    type,
  };

  return true;
}

b32 parse(ParseContext *ctx, AstNodes *nodes) {
  Parser parser;
  parser.messages = ctx->messages;
  parser.tokens   = ctx->tokens;
  parser.nodes    = nodes;

  NodeIndex root;
  return parser.parse_root(&root);
}
