#include "blu.hh"

static constexpr u32 Op_count = BinaryOpKind_max + AssignKind_max;

struct Parser {
  Messages  *messages;
  Tokens    *tokens;
  AstNodes  *nodes;
  TokenIndex at;

  // -

  b32 parse_root(NodeIndex *out);
  b32 parse_block(NodeIndex *out);
  b32 parse_type(NodeIndex *out);
  b32 parse_declaration(NodeIndex *out);
  b32 parse_literal_int(NodeIndex *out);
  b32 parse_literal_string(NodeIndex *out);
  b32 parse_literal_sequence(NodeIndex *out);
  b32 parse_function(NodeIndex *out);
  b32 parse_for(NodeIndex *out);
  b32 parse_break(NodeIndex *out);
  b32 parse_continue(NodeIndex *out);
  b32 parse_return(NodeIndex *out);
  b32 parse_defer(NodeIndex *out);
  b32 parse_if_else(NodeIndex *out);
  b32 parse_const(NodeIndex *out);
  b32 parse_cast(NodeIndex *out);
  b32 parse_base_expression(NodeIndex *out);
  b32 parse_expression(NodeIndex *out, u32 prev_op = Op_count);
  b32 parse_identifier(NodeIndex *out);

  b32 parse_builtin_print(NodeIndex *out);

  // - Helpers -

  template<typename ParseItem>
  b32 parse_comma_separated_items_until(
    SegmentList<NodeIndex> *items, ParseItem parse, TokenKind terminator
  ) {
    while (true) {
      TokenKind tok;
      Try(peek(&tok));

      if (tok == Tok_comma) {
        next();
        Try(peek(&tok));
      }

      if (tok == terminator) {
        break;
      }

      auto item = items->push(nodes->segment_allocator);
      Try(parse(item));
    }

    return true;
  }

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
    auto cur = at;

    TokenKind tok;
    Try(next(&tok));

    if (tok != expected_kind) {
      messages->error(
        "{span} Unexpected token encountered. Expected {tokenkind}, but got {tokenkind}.",
        tokens->span(cur),
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

b32 Parser::parse_builtin_print(NodeIndex *out) {
  auto node_index = nodes->alloc();
  auto start      = at;

  Try(expect_token(Tok_builtin_print));

  AstBuiltin builtin;
  builtin.kind = Builtin_print;
  builtin.args.init();

  Try(expect_token(Tok_paren_open));
  Try(parse_comma_separated_items_until(
    &builtin.args,
    [this](NodeIndex *out) { return parse_expression(out); },
    Tok_paren_close
  ));
  Try(expect_token(Tok_paren_close));

  nodes->set(node_index, {Ast_builtin, {start, at}, {.builtin = builtin}});

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

    Try(peek(&tok));

    if (tok == Tok_bracket_close) {
      next();

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
    } else if (tok == Tok_literal_int) {
      AstTypeArray type_array;
      Try(parse_literal_int(&type_array.size)) Try(expect_token(Tok_bracket_close));
      Try(parse_type(&type_array.base));

      nodes->set(
        node_index,
        {
          Ast_type_array,
          {start, at},
          {.type_array = type_array},
        }
      );
    }
  } break;
  case Tok_paren_open: {
    AstTypeFunction type_function;
    type_function.param_types.init();

    next();
    Try(parse_comma_separated_items_until(
      &type_function.param_types,

      [this](NodeIndex *out) { return parse_type(out); },
      Tok_paren_close
    ));
    Try(expect_token(Tok_paren_close));

    Try(expect_token(Tok_colon));

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

  AstDeclaration declaration{};
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

// NOTE: this function and the parse_literal_int function are basically the same.
b32 Parser::parse_literal_string(NodeIndex *out) {
  auto node_index = nodes->alloc();
  auto start      = at;

  AstAtom literal_string;
  literal_string.token_index = at;

  Try(expect_token(Tok_literal_string));

  nodes->set(
    node_index,
    {
      Ast_literal_string,
      {start, at},
      {.literal_string = literal_string},
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

  Try(expect_token(Tok_brace_open));
  Try(parse_comma_separated_items_until(
    &literal_sequence.items,

    [this](NodeIndex *out) { return parse_expression(out); },
    Tok_brace_close
  ));
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
  function.param_names.init();

  Try(expect_token(Tok_bar));
  Try(parse_comma_separated_items_until(
    &function.param_names,
    [this](NodeIndex *out) { return parse_identifier(out); },
    Tok_bar
  ));
  Try(expect_token(Tok_bar));

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

b32 Parser::parse_for(NodeIndex *out) {
  auto node_index = nodes->alloc();
  auto start      = at;

  TokenKind tok;
  Try(expect_token(Tok_keyword_for, &tok));

  AstFor for_;

  Try(parse_expression(&for_.iterable));

  Try(expect_token(Tok_keyword_do, &tok));

  Try(parse_identifier(&for_.iterator));

  Try(parse_block(&for_.body));

  nodes->set(
    node_index,
    {
      Ast_for,
      {start, at},
      {.for_ = for_},
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

b32 Parser::parse_defer(NodeIndex *out) {
  auto node_index = nodes->alloc();
  auto start      = at;

  Try(expect_token(Tok_keyword_defer));

  AstDefer defer;
  Try(parse_expression(&defer.value));

  nodes->set(
    node_index,
    {
      Ast_defer,
      {start, at},
      {.defer = defer},
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
    if_else.otherwise = NodeIndex::none();

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

  Try(parse_block(&if_else.otherwise));

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

b32 Parser::parse_const(NodeIndex *out) {
  auto node_index = nodes->alloc();
  auto start      = at;

  Try(expect_token(Tok_keyword_const));

  AstConst const_;

  Try(parse_base_expression(&const_.expr));

  nodes->set(
    node_index,
    {
      Ast_const,
      {start, at},
      {.const_ = const_},
    }
  );

  *out = node_index;

  return true;
}

b32 Parser::parse_cast(NodeIndex *out) {
  auto node_index = nodes->alloc();
  auto start      = at;

  Try(expect_token(Tok_keyword_cast));

  AstCast cast;

  Try(expect_token(Tok_paren_open));
  Try(parse_type(&cast.type_dst));
  Try(expect_token(Tok_paren_close));

  Try(parse_base_expression(&cast.value));

  nodes->set(
    node_index,
    {
      Ast_cast,
      {start, at},
      {.cast = cast},
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
  case Tok_keyword_for:      Try(parse_for(&base));            break;
  case Tok_keyword_continue: Try(parse_continue(&base));       break;
  case Tok_keyword_break:    Try(parse_break(&base));          break;
  case Tok_keyword_return:   Try(parse_return(&base));         break;
  case Tok_keyword_defer:    Try(parse_defer(&base));          break;
  case Tok_keyword_if:       Try(parse_if_else(&base));        break;
  case Tok_literal_int:      Try(parse_literal_int(&base));    break;
  case Tok_literal_string:   Try(parse_literal_string(&base)); break;
  case Tok_bar:              Try(parse_function(&base));       break;
  case Tok_builtin_print:    Try(parse_builtin_print(&base));  break;
  case Tok_keyword_const:    Try(parse_const(&base));          break;
  case Tok_keyword_cast:     Try(parse_cast(&base));           break;
    // clang-format on

  case Tok_brace_open: {
    next();
    Try(parse_expression(&base));
    Try(expect_token(Tok_brace_close));
  } break;

  case Tok_dot: {
    next();
    Try(peek(&tok));
    switch (tok) {
    case Tok_brace_open:
      Try(parse_literal_sequence(&base));
      break;
    default:
      return false;
    }
  } break;

  case Tok_identifier: {
    TokenKind tok2;
    if (peek2(&tok2) && tok2 == Tok_colon) {
      Try(parse_declaration(&base));
    } else {
      Try(parse_identifier(&base));
    }
  } break;

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

    // clang-format off
    switch (tok) {
    case Tok_exclamation: unary_op.kind = Not;    break;
    case Tok_minus:       unary_op.kind = Negate; break;

    default: { Unreachable(); } break;
    }
    // clang-format on

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

    if (tok == Tok_bracket_open) {
      auto node_index = nodes->alloc();
      auto start      = at;

      next();

      NodeIndex index_at;
      Try(parse_expression(&index_at));

      AstIndex index = {
        .indexable = base,
        .index_at  = index_at,
      };

      Try(expect_token(Tok_bracket_close));

      nodes->set(
        node_index,
        {
          Ast_index,
          {start, at},
          {.index = index},
        }
      );

      base = node_index;

      continue;
    }

    if (tok == Tok_paren_open) {
      auto node_index = nodes->alloc();
      auto start      = at;

      AstCall call;
      call.args.init();
      call.callee = base;

      next();
      Try(parse_comma_separated_items_until(
        &call.args,

        [this](NodeIndex *out) { return parse_expression(out); },
        Tok_paren_close
      ));
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
    b32       has_peeked = peek(&tok);
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

b32 parse_root(ParseContext *ctx, Tokens *tokens, AstNodes *nodes) {
  Parser parser{};
  parser.messages = ctx->messages;
  parser.tokens   = tokens;
  parser.nodes    = nodes;
  parser.at       = {0};

  NodeIndex root;
  return parser.parse_root(&root);
}
