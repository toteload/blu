#include "blu.hh"

struct Parser {
  StringInterner *strings;
  Vector<AstNode> *nodes;
  Slice<Token> tokens;

  Allocator ref_allocator;
  usize idx;

  void init(
    StringInterner *strings, Allocator ref_allocator, Vector<AstNode> *nodes, Slice<Token> tokens
  );

  b32 parse_module(AstRef *out);

  b32 parse_item(AstRef *out);
  b32 parse_parameter(AstRef *out);
  b32 parse_item_function(AstRef *out);

  b32 parse_type(AstRef *out);

  b32 parse_scope(AstRef *out);

  b32 parse_statement(AstRef *out);
  b32 parse_statement_return(AstRef *out);
  b32 parse_declaration(AstRef *out);
  b32 parse_literal_int(AstRef *out);
  b32 parse_identifier(AstRef *out);

  b32 parse_expression(AstRef *out, BinaryOpKind prev_op = BinaryOpKind_max);
  b32 parse_base_expression(AstRef *out);
  b32 parse_if_else_expression(AstRef *out);
  b32 parse_call(AstRef *out);

  b32 expect_token(TokenKind expected_kind, Token *out = nullptr);
  b32 next(Token *out = nullptr);
  b32 peek(Token *out);
  b32 peek2(Token *out);
  AstRef add_node(AstNode node);

  b32 is_at_end() { return idx == tokens.len; }
};

void Parser::init(
  StringInterner *strings, Allocator ref_allocator, Vector<AstNode> *nodes, Slice<Token> tokens
) {
  this->strings       = strings;
  this->nodes         = nodes;
  this->tokens        = tokens;
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

  if (out != nullptr) {
    *out = tokens[idx];
  }
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

b32 Parser::peek2(Token *out) {
  if (idx + 1 >= tokens.len) {
    return false;
  }

  *out = tokens[idx + 1];

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
  n.kind         = Ast_module;
  n.module.items = items.move();

  AstRef mod = add_node(n);

  *out = mod;

  return true;
}

b32 Parser::parse_item(AstRef *out) { return parse_item_function(out); }

b32 Parser::parse_parameter(AstRef *out) {
  AstRef name_ref;
  Try(parse_identifier(&name_ref));

  Try(expect_token(Tok_colon));

  AstRef type_ref;
  Try(parse_type(&type_ref));

  AstNode n;
  n.kind       = Ast_param;
  n.param.name = name_ref;
  n.param.type = type_ref;

  *out = add_node(n);

  return true;
}

b32 Parser::parse_item_function(AstRef *out) {
  AstRef name_ref;
  Try(parse_identifier(&name_ref));

  Try(expect_token(Tok_colon));
  Try(expect_token(Tok_colon));

  Try(expect_token(Tok_paren_open));

  Vector<AstRef> params;
  params.init(ref_allocator);

  while (!is_at_end()) {
    Token tok;
    Try(peek(&tok));
    if (tok.kind == Tok_paren_close) {
      break;
    }

    AstRef param_ref;
    Try(parse_parameter(&param_ref));

    params.push(param_ref);

    Try(peek(&tok));
    if (tok.kind != Tok_comma) {
      break;
    }

    Try(next(&tok));
  }

  Try(expect_token(Tok_paren_close));

  Try(expect_token(Tok_colon));

  AstRef return_type_ref;
  Try(parse_type(&return_type_ref));

  Try(expect_token(Tok_arrow));

  AstRef body_ref;
  Try(parse_scope(&body_ref));

  AstNode n;
  n.kind                 = Ast_function;
  n.function.params      = params.move();
  n.function.name        = name_ref;
  n.function.return_type = return_type_ref;
  n.function.body        = body_ref;

  *out = add_node(n);

  return true;
}

b32 Parser::parse_scope(AstRef *out) {
  Vector<AstRef> scope;
  scope.init(ref_allocator);

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

  AstNode n;
  n.kind             = Ast_scope;
  n.scope.statements = scope.move();

  *out = add_node(n);

  return true;
}

b32 Parser::parse_declaration(AstRef *out) {
  AstRef identifier;
  Try(parse_identifier(&identifier));

  Try(expect_token(Tok_colon));

  AstRef type;
  Try(parse_type(&type));

  Try(expect_token(Tok_equals));

  AstRef initial_value;
  Try(parse_expression(&initial_value));

  Try(expect_token(Tok_semicolon));

  AstNode n;
  n.kind                      = Ast_declaration;
  n.declaration.name          = identifier;
  n.declaration.type          = type;
  n.declaration.initial_value = initial_value;

  *out = add_node(n);

  return true;
}

b32 Parser::parse_statement(AstRef *out) {
  Token tok;
  Try(peek(&tok));

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

    AstRef value;
    Try(parse_expression(&value));

    Try(expect_token(Tok_semicolon));

    AstNode n;
    n.kind         = Ast_assign;
    n.assign.lhs   = expr;
    n.assign.value = value;

    *out = add_node(n);

    return true;
  }

  Try(expect_token(Tok_semicolon));

  *out = expr;

  return true;
}

b32 Parser::parse_statement_return(AstRef *out) {
  Try(expect_token(Tok_keyword_return));

  AstRef value_ref;
  Try(parse_expression(&value_ref));
  Try(expect_token(Tok_semicolon));

  AstNode n;
  n.kind      = Ast_return;
  n.ret.value = value_ref;

  *out = add_node(n);

  return true;
}

b32 Parser::parse_identifier(AstRef *out) {
  Token tok;
  Try(expect_token(Tok_identifier, &tok));

  StrKey key = strings->add(tok.str());

  AstNode identifier;
  identifier.span           = tok.span;
  identifier.kind           = Ast_identifier;
  identifier.identifier.key = key;

  *out = add_node(identifier);

  return true;
}

b32 Parser::parse_type(AstRef *out) { return parse_identifier(out); }

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
};

Precedence determine_precedence(BinaryOpKind lhs, BinaryOpKind rhs) {
  if (lhs == LessEqual) {
    return Right;
  }

  if (rhs == LessEqual) {
    return Left;
  }

  return Left;
}

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

  if (tok.kind == Tok_minus) {
    next();

    AstRef value;
    Try(parse_expression(&value));

    AstNode n;
    n.kind           = Ast_unary_op;
    n.unary_op.kind  = Negate;
    n.unary_op.value = value;

    *out = add_node(n);

    return true;
  }

  if (tok.kind == Tok_identifier) {
    return parse_identifier(out);
  }

  if (tok.kind == Tok_literal_int) {
    return parse_literal_int(out);
  }

  return false;
}

b32 Parser::parse_expression(AstRef *out, BinaryOpKind prev_op) {
  AstRef lhs;
  Try(parse_base_expression(&lhs));

  while (!is_at_end()) {
    Token tok;
    if (!peek(&tok) || tok.kind != Tok_paren_open) {
      break;
    }

    next();

    Vector<AstRef> arguments;
    arguments.init(ref_allocator);

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

    AstNode n;
    n.kind           = Ast_call;
    n.call.f         = lhs;
    n.call.arguments = arguments;

    lhs = add_node(n);
  }

  while (true) {
    Token tok;
    Try(peek(&tok));

    BinaryOpKind op = BinaryOpKind_max;
    switch (tok.kind) {
    case Tok_minus:
      op = Sub;
      break;
    case Tok_plus:
      op = Add;
      break;
    case Tok_less_equal_than:
      op = LessEqual;
      break;
    default: {
      *out = lhs;
      return true;
    }
    }

    Precedence precedence = Right;
    if (prev_op != BinaryOpKind_max) {
      precedence = determine_precedence(prev_op, op);
    }

    if (precedence == Left) {
      *out = lhs;
      return true;
    }

    next();

    AstRef rhs;
    Try(parse_expression(&rhs, op));

    AstNode n;
    n.kind           = Ast_binary_op;
    n.binary_op.kind = op;
    n.binary_op.lhs  = lhs;
    n.binary_op.rhs  = rhs;

    AstRef binop = add_node(n);

    lhs = binop;
  }

  *out = lhs;

  return true;
}

b32 Parser::parse_if_else_expression(AstRef *out) {
  Try(expect_token(Tok_keyword_if));

  AstRef cond;
  Try(parse_expression(&cond));

  AstRef then;
  Try(parse_scope(&then));

  Token tok;
  Try(peek(&tok));
  if (tok.kind != Tok_keyword_else) {
    AstNode n;
    n.kind              = Ast_if_else;
    n.if_else.cond      = cond;
    n.if_else.then      = then;
    n.if_else.otherwise = nil;

    *out = add_node(n);

    return true;
  }

  Try(next(&tok));

  AstRef otherwise;
  Try(parse_scope(&otherwise));

  AstNode n;
  n.kind              = Ast_if_else;
  n.if_else.cond      = cond;
  n.if_else.then      = then;
  n.if_else.otherwise = otherwise;

  *out = add_node(n);

  return true;
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
