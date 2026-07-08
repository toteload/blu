#include "blu.h"
#include "ast.h"
#include "tokens.h"
#include "source_file.h"

void nodes_init(AstNodes *nodes) {
  arena_init(&nodes->kinds, &(ArenaOptions){
    .reserve_size        = MiB(1),
    .initial_commit_size = KiB(16),
  });

  arena_init(&nodes->spans, &(ArenaOptions){
    .reserve_size        = 8 * MiB(1),
    .initial_commit_size = 8 * KiB(16),
  });

  arena_init(&nodes->datas, &(ArenaOptions){
    .reserve_size        = 4 * MiB(1),
    .initial_commit_size = 4 * KiB(16),
  });

  arena_init(&nodes->extra, &(ArenaOptions){
    .reserve_size        = 128 * MiB(1),
    .initial_commit_size = 128 * KiB(16),
  });

  arena_push_array(u8,        &nodes->kinds, 1);
  arena_push_array(SpanToken, &nodes->spans, 1);
  arena_push_array(u32,       &nodes->datas, 1);

  nodes->offset = 1;
}

void nodes_deinit(AstNodes *nodes) {
  arena_deinit(&nodes->kinds);
  arena_deinit(&nodes->spans);
  arena_deinit(&nodes->datas);
  arena_deinit(&nodes->extra);

  zero_struct(AstNodes, nodes);
}

AstIndex nodes_begin(AstNodes *nodes) {
  Unused(nodes);
  return 1;
}

AstIndex nodes_end(AstNodes *nodes) {
  return nodes->offset;
}

AstIndex nodes_alloc(AstNodes *nodes) {
  AstIndex idx = nodes->offset;
  nodes->offset += 1;
  arena_push_array(u8, &nodes->kinds, 1);
  arena_push_array(SpanToken, &nodes->spans, 1);
  arena_push_array(u32, &nodes->datas, 1);
  return idx;
}

u8 *nodes_kind(AstNodes *nodes, AstIndex idx) {
  return Cast(u8*, nodes->kinds.base) + idx;
}

SpanToken *nodes_span(AstNodes *nodes, AstIndex idx) {
  return Cast(SpanToken*, nodes->spans.base) + idx;
}

void *nodes_data(AstNodes *nodes, AstIndex idx) {
  u32 offset = Cast(u32*, nodes->datas.base)[idx];
  return ptr_offset(nodes->extra.base, offset);
}

internal void *nodes_push_data_raw(AstNodes *nodes, AstIndex idx, usize size, u32 align) {
  void *p = arena_push(&nodes->extra, size, align);
  u32 offset = Cast(u32, ptr_diff(p, nodes->extra.base));
  Cast(u32*, nodes->datas.base)[idx] = offset;
  return p;
}

#define nodes_push_data(nodes, type, idx) nodes_push_data_raw(nodes, idx, sizeof(type), Align_of(type))

#define SEGMENTLIST_NAME            NodeIndexList
#define SEGMENTLIST_TYPE            NodeIndex
#define SEGMENTLIST_FUNCTION_PREFIX list
#define SEGMENTLIST_MIN_SIZE_LOG2   3
#define SEGMENTLIST_SEGMENT_COUNT   24
#define SEGMENTLIST_LINKAGE         internal 
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

#define Op_count (BinaryOpKind_max + AssignKind_max)

#define Try(e)                                                                                     \
  if (!(e)) {                                                                                      \
    return False;                                                                                  \
  }

typedef struct {
  Source     *source;
  Tokens     *tokens;
  AstNodes   *nodes;
  TokenIndex  at;
} Parser;

internal b32 parse_source(Parser *parser, NodeIndex *out);
internal b32 parse_mod_section(Parser *parser, NodeIndex *out);
internal b32 parse_declaration(Parser *parser, NodeIndex *out);
internal b32 parse_block(Parser *parser, NodeIndex *out);
internal b32 parse_type(Parser *parser, NodeIndex *out);
internal b32 parse_function(Parser *parser, NodeIndex *out);
internal b32 parse_cast(Parser *parser, NodeIndex *out);
internal b32 parse_as(Parser *parser, NodeIndex *out);
internal b32 parse_if_else(Parser *parser, NodeIndex *out);
internal b32 parse_base_expression(Parser *parser, NodeIndex *out);
internal b32 parse_expression(Parser *parser, NodeIndex *out);
internal b32 parse_literal_int(Parser *parser, NodeIndex *out);
internal b32 parse_literal_string(Parser *parser, NodeIndex *out);
internal b32 parse_for(Parser *parser, NodeIndex *out);
internal b32 parse_defer(Parser *parser, NodeIndex *out);
internal b32 parse_const(Parser *parser, NodeIndex *out);
internal b32 parse_identifier(Parser *parser, NodeIndex *out);
internal b32 parse_param(Parser *parser, NodeIndex *out);
internal b32 parse_builtin_print(Parser *parser, NodeIndex *out);

internal b32 parse_expression_impl(Parser *parser, NodeIndex *out, u32 prev_op);

internal b32 is_token_index_past_end(Tokens *tokens, TokenIndex idx) {
  return idx >= tokens_end(tokens);
}

internal b32 is_parser_past_end(Parser *parser) {
  return is_token_index_past_end(parser->tokens, parser->at);
}

b32 next(Parser *parser, u8 *token_kind) {
  if (is_parser_past_end(parser)) {
    return False;
  }

  *token_kind = *tokens_kind(parser->tokens, parser->at);

  parser->at += 1;

  return True;
}

internal b32 expect_token(Parser *parser, u8 expected_token_kind) {
  u8 tok;
  b32 has_next = next(parser, &tok);

  if (!has_next) {
    error(
      parser->source,
      (MessageLocation){ .kind = MessageLocation_end_of_file },
      string_lit("Expected a token, but encountered end of source.")
    );
    return False;
  }

  if (tok != expected_token_kind) {
    error(
      parser->source,
      (MessageLocation){ .kind = MessageLocation_token_index, .data.token_index = parser->at - 1 },
      string_lit("Expected token {tok}, but got token {tok}."), expected_token_kind, tok
    );
    return False;
  }

  return True;
}

internal b32 peek(Parser *parser, u8 *token_kind) {
  if (is_parser_past_end(parser)) {
    return False;
  }

  *token_kind = *tokens_kind(parser->tokens, parser->at);

  return True;
}

internal b32 peek_or_error(Parser *parser, u8 *token_kind) {
  b32 has_peeked = peek(parser, token_kind);
  if (!has_peeked) {
    error(
      parser->source,
      (MessageLocation){ .kind = MessageLocation_end_of_file },
      string_lit("Expected a token but got end of source.")
    );
    return False;
  }

  return True;
}

internal b32 peek2(Parser *parser, u8 *token_kind) {
  TokenIndex lookahead = parser->at + 1;

  if (is_token_index_past_end(parser->tokens, lookahead)) {
    return False;
  }

  *token_kind = *tokens_kind(parser->tokens, lookahead);

  return True;
}

internal b32 consume_if_match(Parser *parser, u8 token_kind_match) {
  u8 tok;
  Try(peek(parser, &tok));

  if (tok == token_kind_match) {
    next(parser, &tok);
    return True;
  }

  return False;
}

internal NodeIndex node_index_from_ast(AstIndex idx) {
  return (NodeIndex){ .kind = NodeIndex_ast, .idx.ast = idx, };
}

typedef b32 (*ParseItemFn)(Parser *parser, NodeIndex *out);

internal b32 parse_comma_separated_items_until(
  Parser *parser, NodeIndexList *items, ParseItemFn parse, u8 terminator
) {
  while (True) {
    consume_if_match(parser, Tok_comma);

    u8  tok;
    b32 has_next = peek(parser, &tok);

    if (!has_next || tok == terminator) {
      break;
    }

    NodeIndex *item = list_push(items, &parser->nodes->extra);
    Try(parse(parser, item));
  }

  return True;
}

internal b32 parse_source(Parser *parser, NodeIndex *out) {
  AstIndex idx = nodes_alloc(parser->nodes);
  TokenIndex start = parser->at;

  AstSource *source = nodes_push_data(parser->nodes, AstSource, idx);
  zero_struct(AstSource, source);

  while (!is_parser_past_end(parser)) {
    NodeIndex *section = list_push(&source->items, &parser->nodes->extra);
    Try(parse_mod_section(parser, section));
  }

  *nodes_kind(parser->nodes, idx) = Ast_source;
  *nodes_span(parser->nodes, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = (NodeIndex){ .kind = NodeIndex_ast, .idx.ast = idx, };

  return True;
}

internal b32 parse_mod_section(Parser *parser, NodeIndex *out) {
  AstIndex idx = nodes_alloc(parser->nodes);
  TokenIndex start = parser->at;

  Try(expect_token(parser, Tok_keyword_mod));

  AstModSection *section = nodes_push_data(parser->nodes, AstModSection, idx);
  zero_struct(AstModSection, section);

  Try(parse_identifier(parser, &section->name));

  while (!is_parser_past_end(parser)) {
    u8 tok;
    peek(parser, &tok);
    if (tok == Tok_keyword_mod) {
      break;
    }

    NodeIndex *decl = list_push(&section->items, &parser->nodes->extra);
    Try(parse_declaration(parser, decl));
  }

  *nodes_kind(parser->nodes, idx) = Ast_mod_section;
  *nodes_span(parser->nodes, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = (NodeIndex){ .kind = NodeIndex_ast, .idx.ast = idx, };

  return True;
}

internal b32 parse_builtin_print(Parser *parser, NodeIndex *out) {
  AstIndex   idx   = nodes_alloc(parser->nodes);
  TokenIndex start = parser->at;

  Try(expect_token(parser, Tok_builtin_print));

  AstBuiltin *builtin = nodes_push_data(parser->nodes, AstBuiltin, idx);
  zero_struct(AstBuiltin, builtin);
  builtin->kind = Builtin_print;

  Try(expect_token(parser, Tok_paren_open));
  Try(parse_comma_separated_items_until(parser, &builtin->args, parse_expression, Tok_paren_close));
  Try(expect_token(parser, Tok_paren_close));

  *nodes_kind(parser->nodes, idx) = Ast_builtin;
  *nodes_span(parser->nodes, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = node_index_from_ast(idx);

  return True;
}

internal b32 parse_block(Parser *parser, NodeIndex *out) {
  AstIndex   idx   = nodes_alloc(parser->nodes);
  TokenIndex start = parser->at;

  AstBlock *block = nodes_push_data(parser->nodes, AstBlock, idx);
  zero_struct(AstBlock, block);

  Try(expect_token(parser, Tok_brace_open));

  while (!is_parser_past_end(parser)) {
    u8 tok;
    Try(peek(parser, &tok));
    if (tok == Tok_brace_close) {
      break;
    }

    NodeIndex *e = list_push(&block->items, &parser->nodes->extra);
    Try(parse_expression(parser, e));
  }

  Try(expect_token(parser, Tok_brace_close));

  *nodes_kind(parser->nodes, idx) = Ast_block;
  *nodes_span(parser->nodes, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = node_index_from_ast(idx);

  return True;
}

internal b32 parse_type(Parser *parser, NodeIndex *out) {
  AstIndex   idx   = nodes_alloc(parser->nodes);
  TokenIndex start = parser->at;

  u8 tok;
  Try(peek(parser, &tok));

  switch (tok) {
  case Tok_bracket_open: {
    u8 ignored;
    next(parser, &ignored);

    Try(peek(parser, &tok));

    if (tok == Tok_bracket_close) {
      next(parser, &ignored);

      AstTypeSlice *type_slice = nodes_push_data(parser->nodes, AstTypeSlice, idx);
      Try(parse_type(parser, &type_slice->base));

      *nodes_kind(parser->nodes, idx) = Ast_type_slice;
    } else if (tok == Tok_literal_int) {
      AstTypeArray *type_array = nodes_push_data(parser->nodes, AstTypeArray, idx);
      Try(parse_literal_int(parser, &type_array->size));
      Try(expect_token(parser, Tok_bracket_close));
      Try(parse_type(parser, &type_array->base));

      *nodes_kind(parser->nodes, idx) = Ast_type_array;
    } else {
      // TODO: you could put any expression between the brackets really
      Panic();
    }
  } break;
  case Tok_paren_open: {
    AstTypeFunction *type_function = nodes_push_data(parser->nodes, AstTypeFunction, idx);
    zero_struct(AstTypeFunction, type_function);

    u8 ignored;
    next(parser, &ignored);
    Try(parse_comma_separated_items_until(
      parser, &type_function->param_types, parse_type, Tok_paren_close
    ));
    Try(expect_token(parser, Tok_paren_close));

    Try(parse_type(parser, &type_function->return_type));

    *nodes_kind(parser->nodes, idx) = Ast_type_function;
  } break;
  case Tok_identifier: {
    TokenIndex identifier = parser->at;

    u8 ignored;
    next(parser, &ignored);

    TokenIndex *p = nodes_push_data(parser->nodes, TokenIndex, idx);
    *p = identifier;

    *nodes_kind(parser->nodes, idx) = Ast_identifier;
  } break;
  default:
    error(
      parser->source,
      (MessageLocation){ .kind = MessageLocation_token_index, .data.token_index = parser->at },
      string_lit("Unexpected token {tok} encountered in type expression."), tok
    );
    return False;
  }

  *nodes_span(parser->nodes, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = node_index_from_ast(idx);

  return True;
}

internal b32 parse_declaration(Parser *parser, NodeIndex *out) {
  AstIndex   idx   = nodes_alloc(parser->nodes);
  TokenIndex start = parser->at;

  AstDeclaration *declaration = nodes_push_data(parser->nodes, AstDeclaration, idx);
  zero_struct(AstDeclaration, declaration);
  declaration->name = parser->at;

  Try(expect_token(parser, Tok_identifier));

  Try(expect_token(parser, Tok_colon));

  u8 tok;
  Try(peek_or_error(parser, &tok));

  if (tok != Tok_equals) {
    Try(parse_type(parser, &declaration->type));
  } else {
    declaration->type = (NodeIndex){ .kind = NodeIndex_none };
  }

  Try(expect_token(parser, Tok_equals));

  Try(parse_expression(parser, &declaration->value));

  *nodes_kind(parser->nodes, idx) = Ast_declaration;
  *nodes_span(parser->nodes, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = node_index_from_ast(idx);

  return True;
}

internal b32 parse_literal_int(Parser *parser, NodeIndex *out) {
  AstIndex   idx   = nodes_alloc(parser->nodes);
  TokenIndex start = parser->at;

  TokenIndex literal_int = parser->at;

  Try(expect_token(parser, Tok_literal_int));

  TokenIndex *p = nodes_push_data(parser->nodes, TokenIndex, idx);
  *p = literal_int;

  *nodes_kind(parser->nodes, idx) = Ast_literal_int;
  *nodes_span(parser->nodes, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = node_index_from_ast(idx);

  return True;
}

// NOTE: this function and the parse_literal_int function are basically the same.
internal b32 parse_literal_string(Parser *parser, NodeIndex *out) {
  AstIndex   idx   = nodes_alloc(parser->nodes);
  TokenIndex start = parser->at;

  TokenIndex literal_string = parser->at;

  Try(expect_token(parser, Tok_literal_string));

  TokenIndex *p = nodes_push_data(parser->nodes, TokenIndex, idx);
  *p = literal_string;

  *nodes_kind(parser->nodes, idx) = Ast_literal_string;
  *nodes_span(parser->nodes, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = node_index_from_ast(idx);

  return True;
}

internal b32 parse_param(Parser *parser, NodeIndex *out) {
  AstIndex   idx   = nodes_alloc(parser->nodes);
  TokenIndex start = parser->at;

  AstParam *param = nodes_push_data(parser->nodes, AstParam, idx);
  zero_struct(AstParam, param);
  param->name = parser->at;

  Try(expect_token(parser, Tok_identifier));

  if (consume_if_match(parser, Tok_colon)) {
    Try(parse_type(parser, &param->type));
  }

  *nodes_kind(parser->nodes, idx) = Ast_param;
  *nodes_span(parser->nodes, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = node_index_from_ast(idx);

  return True;
}

internal b32 parse_function(Parser *parser, NodeIndex *out) {
  AstIndex   idx   = nodes_alloc(parser->nodes);
  TokenIndex start = parser->at;

  AstFunction *function = nodes_push_data(parser->nodes, AstFunction, idx);
  zero_struct(AstFunction, function);

  Try(expect_token(parser, Tok_bar));
  Try(parse_comma_separated_items_until(parser, &function->params, parse_param, Tok_bar));
  Try(expect_token(parser, Tok_bar));

  if (consume_if_match(parser, Tok_colon)) {
    Try(parse_type(parser, &function->return_type));
  }

  Try(parse_expression(parser, &function->body));

  *nodes_kind(parser->nodes, idx) = Ast_function;
  *nodes_span(parser->nodes, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = node_index_from_ast(idx);

  return True;
}

internal b32 parse_for(Parser *parser, NodeIndex *out) {
  AstIndex   idx   = nodes_alloc(parser->nodes);
  TokenIndex start = parser->at;

  Try(expect_token(parser, Tok_keyword_for));

  AstFor *for_ = nodes_push_data(parser->nodes, AstFor, idx);

  Try(parse_expression(parser, &for_->iterable));

  Try(expect_token(parser, Tok_keyword_do));

  Try(parse_identifier(parser, &for_->iterator));

  Try(parse_block(parser, &for_->body));

  *nodes_kind(parser->nodes, idx) = Ast_for;
  *nodes_span(parser->nodes, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = node_index_from_ast(idx);

  return True;
}

internal b32 parse_defer(Parser *parser, NodeIndex *out) {
  AstIndex   idx   = nodes_alloc(parser->nodes);
  TokenIndex start = parser->at;

  Try(expect_token(parser, Tok_keyword_defer));

  AstDefer *defer = nodes_push_data(parser->nodes, AstDefer, idx);
  Try(parse_expression(parser, &defer->value));

  *nodes_kind(parser->nodes, idx) = Ast_defer;
  *nodes_span(parser->nodes, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = node_index_from_ast(idx);

  return True;
}

internal b32 parse_if_else(Parser *parser, NodeIndex *out) {
  AstIndex   idx   = nodes_alloc(parser->nodes);
  TokenIndex start = parser->at;

  Try(expect_token(parser, Tok_keyword_if));

  AstIfElse *if_else = nodes_push_data(parser->nodes, AstIfElse, idx);

  Try(parse_expression(parser, &if_else->cond));

  Try(parse_block(parser, &if_else->then));

  u8 tok;
  Try(peek(parser, &tok));
  if (tok != Tok_keyword_else) {
    if_else->otherwise = (NodeIndex){0};

    *nodes_kind(parser->nodes, idx) = Ast_if_else;
    *nodes_span(parser->nodes, idx) = (SpanToken){ .start = start, .end = parser->at, };

    *out = node_index_from_ast(idx);

    return True;
  }

  u8 ignored;
  next(parser, &ignored);

  Try(parse_block(parser, &if_else->otherwise));

  *nodes_kind(parser->nodes, idx) = Ast_if_else;
  *nodes_span(parser->nodes, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = node_index_from_ast(idx);

  return True;
}

internal b32 parse_const(Parser *parser, NodeIndex *out) {
  AstIndex   idx   = nodes_alloc(parser->nodes);
  TokenIndex start = parser->at;

  Try(expect_token(parser, Tok_keyword_const));

  AstConst *const_ = nodes_push_data(parser->nodes, AstConst, idx);

  Try(parse_base_expression(parser, &const_->expr));

  *nodes_kind(parser->nodes, idx) = Ast_const;
  *nodes_span(parser->nodes, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = node_index_from_ast(idx);

  return True;
}

internal b32 parse_cast(Parser *parser, NodeIndex *out) {
  AstIndex   idx   = nodes_alloc(parser->nodes);
  TokenIndex start = parser->at;

  Try(expect_token(parser, Tok_keyword_cast));

  AstCast *cast = nodes_push_data(parser->nodes, AstCast, idx);

  Try(expect_token(parser, Tok_paren_open));
  Try(parse_type(parser, &cast->type_dst));
  Try(expect_token(parser, Tok_paren_close));

  Try(parse_base_expression(parser, &cast->value));

  *nodes_kind(parser->nodes, idx) = Ast_cast;
  *nodes_span(parser->nodes, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = node_index_from_ast(idx);

  return True;
}

internal b32 parse_as(Parser *parser, NodeIndex *out) {
  AstIndex   idx   = nodes_alloc(parser->nodes);
  TokenIndex start = parser->at;

  Try(expect_token(parser, Tok_keyword_as));

  AstCast *cast = nodes_push_data(parser->nodes, AstCast, idx);

  Try(expect_token(parser, Tok_paren_open));
  Try(parse_type(parser, &cast->type_dst));
  Try(expect_token(parser, Tok_paren_close));

  Try(parse_base_expression(parser, &cast->value));

  *nodes_kind(parser->nodes, idx) = Ast_as;
  *nodes_span(parser->nodes, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = node_index_from_ast(idx);

  return True;
}

internal b32 parse_identifier(Parser *parser, NodeIndex *out) {
  AstIndex   idx   = nodes_alloc(parser->nodes);
  TokenIndex start = parser->at;

  TokenIndex identifier = parser->at;

  Try(expect_token(parser, Tok_identifier));

  TokenIndex *p = nodes_push_data(parser->nodes, TokenIndex, idx);
  *p = identifier;

  *nodes_kind(parser->nodes, idx) = Ast_identifier;
  *nodes_span(parser->nodes, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = node_index_from_ast(idx);

  return True;
}

internal b32 parse_base_expression(Parser *parser, NodeIndex *out) {
  u8  tok;
  b32 has_next_token = peek(parser, &tok);
  if (!has_next_token) {
    error(
      parser->source,
      (MessageLocation){ .kind = MessageLocation_end_of_file },
      string_lit("Unexpected end of source, while trying to parse base expression.")
    );
    return False;
  }

  NodeIndex base;
  switch (tok) {
    // clang-format off
  case Tok_keyword_for:      Try(parse_for(parser, &base));            break;
  case Tok_keyword_defer:    Try(parse_defer(parser, &base));          break;
  case Tok_keyword_if:       Try(parse_if_else(parser, &base));        break;
  case Tok_literal_int:      Try(parse_literal_int(parser, &base));    break;
  case Tok_literal_string:   Try(parse_literal_string(parser, &base)); break;
  case Tok_bar:              Try(parse_function(parser, &base));       break;
  case Tok_builtin_print:    Try(parse_builtin_print(parser, &base));  break;
  case Tok_keyword_const:    Try(parse_const(parser, &base));          break;
  case Tok_keyword_cast:     Try(parse_cast(parser, &base));           break;
  case Tok_keyword_as:       Try(parse_as(parser, &base));             break;
  case Tok_brace_open:       Try(parse_block(parser, &base));          break;
    // clang-format on

  case Tok_dot: {
    u8 ignored;
    next(parser, &ignored);
    Try(peek(parser, &tok));
    switch (tok) {
    default:
      error(
        parser->source,
        (MessageLocation){ .kind = MessageLocation_token_index, .data.token_index = parser->at },
        string_lit("Unexpected token {tok} encountered after '.' in expression."), tok
      );
      return False;
    }
  } break;

  case Tok_identifier: {
    u8 tok2;
    if (peek2(parser, &tok2) && tok2 == Tok_colon) {
      Try(parse_declaration(parser, &base));
    } else {
      Try(parse_identifier(parser, &base));
    }
  } break;

  case Tok_star:
  case Tok_bracket_open: {
    Try(parse_type(parser, &base));
  } break;

  case Tok_exclamation:
  case Tok_minus: {
    AstIndex   ast_index = nodes_alloc(parser->nodes);
    TokenIndex start     = parser->at;

    u8 ignored;
    next(parser, &ignored);

    AstUnaryOp *unary_op = nodes_push_data(parser->nodes, AstUnaryOp, ast_index);

    // clang-format off
    switch (tok) {
    case Tok_exclamation: unary_op->op_kind = Not;    break;
    case Tok_minus:       unary_op->op_kind = Negate; break;

    default: { Unreachable(); } break;
    }
    // clang-format on

    Try(parse_expression(parser, &unary_op->value));

    *nodes_kind(parser->nodes, ast_index) = Ast_unary_op;
    *nodes_span(parser->nodes, ast_index) = (SpanToken){ .start = start, .end = parser->at, };

    base = node_index_from_ast(ast_index);
  } break;

  default:
    error(
      parser->source,
      (MessageLocation){ .kind = MessageLocation_token_index, .data.token_index = parser->at },
      string_lit("Unexpected token {tok} encountered at start of expression."), tok
    );
    return False;
  }

  while (!is_parser_past_end(parser)) {
    u8 tok;
    peek(parser, &tok);

    if (tok == Tok_bracket_open) {
      AstIndex   ast_index = nodes_alloc(parser->nodes);
      TokenIndex start     = parser->at;

      u8 ignored;
      next(parser, &ignored);

      AstIndexData *index = nodes_push_data(parser->nodes, AstIndexData, ast_index);
      index->indexable = base;
      Try(parse_expression(parser, &index->index_at));

      Try(expect_token(parser, Tok_bracket_close));

      *nodes_kind(parser->nodes, ast_index) = Ast_index;
      *nodes_span(parser->nodes, ast_index) = (SpanToken){ .start = start, .end = parser->at, };

      base = node_index_from_ast(ast_index);

      continue;
    }

    if (tok == Tok_paren_open) {
      AstIndex   ast_index = nodes_alloc(parser->nodes);
      TokenIndex start     = parser->at;

      AstCall *call = nodes_push_data(parser->nodes, AstCall, ast_index);
      zero_struct(AstCall, call);
      call->callee = base;

      u8 ignored;
      next(parser, &ignored);
      Try(parse_comma_separated_items_until(parser, &call->args, parse_expression, Tok_paren_close));
      Try(expect_token(parser, Tok_paren_close));

      *nodes_kind(parser->nodes, ast_index) = Ast_call;
      *nodes_span(parser->nodes, ast_index) = (SpanToken){ .start = start, .end = parser->at, };

      base = node_index_from_ast(ast_index);

      continue;
    }

    break;
  }

  *out = base;

  return True;
}

// clang-format off
internal const u8 op_precedence_group[Op_count] = {
  10, 10, 10,
  20, 20,
  30, 30,
  40, 40, 40,
  50, 50, 50, 50, 50, 50,
  60, 60,
  200,
};
// clang-format on

enum Precedence {
  Prec_left,
  Prec_right,
};

internal enum Precedence determine_precedence(u32 lhs, u32 rhs) {
  u8 left_group  = op_precedence_group[lhs];
  u8 right_group = op_precedence_group[rhs];

  if (left_group <= right_group) {
    return Prec_left;
  }

  return Prec_right;
}

internal b32 parse_expression_impl(Parser *parser, NodeIndex *out, u32 prev_op) {
  NodeIndex lhs;
  Try(parse_base_expression(parser, &lhs));

  while (True) {
    u8  tok;
    b32 has_peeked = peek(parser, &tok);
    if (!has_peeked) {
      *out = lhs;
      return True;
    }

    u8 op = Op_count;

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

    default: { *out = lhs; return True; }
    }
    // clang-format on

    enum Precedence precedence = Prec_right;
    if (prev_op != Op_count) {
      precedence = determine_precedence(prev_op, op);
    }

    if (precedence == Prec_left) {
      *out = lhs;
      return True;
    }

    AstIndex   ast_index = nodes_alloc(parser->nodes);
    TokenIndex start     = parser->at;

    u8 ignored;
    next(parser, &ignored);

    NodeIndex rhs;
    Try(parse_expression_impl(parser, &rhs, op));

    if (op >= BinaryOpKind_max) {
      AstAssign *assign = nodes_push_data(parser->nodes, AstAssign, ast_index);
      assign->assign_kind = Assign_normal;
      assign->lhs         = lhs;
      assign->value       = rhs;

      *nodes_kind(parser->nodes, ast_index) = Ast_assign;
      *nodes_span(parser->nodes, ast_index) = (SpanToken){ .start = start, .end = parser->at, };
    } else {
      AstBinaryOp *binary_op = nodes_push_data(parser->nodes, AstBinaryOp, ast_index);
      binary_op->op_kind = op;
      binary_op->lhs     = lhs;
      binary_op->rhs     = rhs;

      *nodes_kind(parser->nodes, ast_index) = Ast_binary_op;
      *nodes_span(parser->nodes, ast_index) = (SpanToken){ .start = start, .end = parser->at, };
    }

    lhs = node_index_from_ast(ast_index);
  }

  *out = lhs;

  return True;
}

internal b32 parse_expression(Parser *parser, NodeIndex *out) {
  return parse_expression_impl(parser, out, Op_count);
}

b32 source_parse(Source *source) {
  Parser parser = {
    .source = source,
    .tokens = &source->tokens,
    .nodes  = &source->ast,
    .at     = tokens_begin(&source->tokens),
  };

  NodeIndex ignored;
  return parse_source(&parser, &ignored);
}

String ast_string[] = {
  [Ast_source]         = string_lit("source"),
  [Ast_mod_section]    = string_lit("mod-section"),
  [Ast_block]          = string_lit("block"),          
  [Ast_type_slice]     = string_lit("type-slice"),
  [Ast_type_array]     = string_lit("type-array"),  
  [Ast_type_function]  = string_lit("type-function"),  
  [Ast_builtin]        = string_lit("builtin"),
  [Ast_declaration]    = string_lit("declaration"), 
  [Ast_assign]         = string_lit("assign"),
  [Ast_literal_int]    = string_lit("literal-int"),
  [Ast_literal_string] = string_lit("literal-string"),
  [Ast_identifier]     = string_lit("identifier"),
  [Ast_call]           = string_lit("call"),
  [Ast_index]          = string_lit("index"),
  [Ast_unary_op]       = string_lit("unary-op"),
  [Ast_binary_op]      = string_lit("binary-op"),
  [Ast_function]       = string_lit("function"),
  [Ast_param]          = string_lit("param"),
  [Ast_if_else]        = string_lit("if-else"),
  [Ast_for]            = string_lit("for"),
  [Ast_defer]          = string_lit("defer"),
  [Ast_const]          = string_lit("const"),
  [Ast_cast]           = string_lit("cast"),
  [Ast_as]             = string_lit("as"),
};

String ast_kind_string(u8 kind) {
  if (kind >= Ast_kind_max) {
    return string_lit("<illegal-ast-kind>");
  }

  return ast_string[kind];
}
