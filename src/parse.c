#include "blu.h"
#include "ast.h"
#include "tokens.h"
#include "source_file.h"

#define SEGMENTLIST_NAME            AstIndexList
#define SEGMENTLIST_TYPE            AstIndex
#define SEGMENTLIST_FUNCTION_PREFIX list
#define SEGMENTLIST_MIN_SIZE_LOG2   3
#define SEGMENTLIST_SEGMENT_COUNT   24
#define SEGMENTLIST_LINKAGE         internal
#define SEGMENTLIST_OUTPUT_TYPES
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

#define SEGMENTLIST_NAME            KindList
#define SEGMENTLIST_TYPE            u8
#define SEGMENTLIST_FUNCTION_PREFIX kindlist
#define SEGMENTLIST_LINKAGE         internal
#define SEGMENTLIST_MIN_SIZE_LOG2   8
#define SEGMENTLIST_SEGMENT_COUNT   24
#define SEGMENTLIST_OUTPUT_TYPES
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

#define SEGMENTLIST_NAME            SpanList
#define SEGMENTLIST_TYPE            SpanToken
#define SEGMENTLIST_FUNCTION_PREFIX spanlist
#define SEGMENTLIST_LINKAGE         internal
#define SEGMENTLIST_MIN_SIZE_LOG2   8
#define SEGMENTLIST_SEGMENT_COUNT   24
#define SEGMENTLIST_OUTPUT_TYPES
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

#define SEGMENTLIST_NAME            DataList
#define SEGMENTLIST_TYPE            u32
#define SEGMENTLIST_FUNCTION_PREFIX datalist
#define SEGMENTLIST_LINKAGE         internal
#define SEGMENTLIST_MIN_SIZE_LOG2   8
#define SEGMENTLIST_SEGMENT_COUNT   24
#define SEGMENTLIST_OUTPUT_TYPES
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

#define Op_count (BinaryOpKind_max + AssignKind_max)

#define Try(e)                                                                                     \
  if (!(e)) {                                                                                      \
    return False;                                                                                  \
  }

typedef struct {
  Tokens      *tokens;
  TokenIndex   at;

  MessageSink *msg_sink;

  KindList     kinds;
  SpanList     spans;
  DataList     datas;

  // The node lists above are backed by `scratch` and are copied into flat arrays in `arena` when
  // parsing is done. Node payloads ('extra' data) go directly into `arena`. Variable-length
  // children are collected in a scratch-backed `AstIndexList` while parsing and are copied into a
  // trailing array in the payload once the node is complete, so the payloads stay packed.
  Arena *arena;
  Arena *scratch;
} Parser;

internal AstIndex node_alloc(Parser *parser) {
  AstIndex idx = Cast(AstIndex, parser->kinds.len);
  kindlist_append(&parser->kinds, parser->scratch, 0);
  spanlist_append(&parser->spans, parser->scratch, (SpanToken){0});
  datalist_append(&parser->datas, parser->scratch, 0);
  return idx;
}

internal u8 *node_kind(Parser *parser, AstIndex idx) {
  return kindlist_ptr_at_unchecked(&parser->kinds, idx);
}

internal SpanToken *node_span(Parser *parser, AstIndex idx) {
  return spanlist_ptr_at_unchecked(&parser->spans, idx);
}

internal void *node_push_data_raw(Parser *parser, AstIndex idx, usize size, u32 align) {
  void *p = arena_push(parser->arena, size, align);
  *datalist_ptr_at_unchecked(&parser->datas, idx) = Cast(u32, ptr_diff(p, parser->arena->base));
  return p;
}

#define node_push_data(parser, type, idx) node_push_data_raw(parser, idx, sizeof(type), Align_of(type))

// For payloads with a trailing `AstIndex` array holding `n` children.
#define node_push_data_flex(parser, type, n, idx) \
  node_push_data_raw(parser, idx, sizeof(type) + (n) * sizeof(AstIndex), Align_of(type))

internal b32 parse_source(Parser *parser, AstIndex *out);
internal b32 parse_mod_section(Parser *parser, AstIndex *out);
internal b32 parse_declaration(Parser *parser, AstIndex *out);
internal b32 parse_block(Parser *parser, AstIndex *out);
internal b32 parse_type(Parser *parser, AstIndex *out);
internal b32 parse_function(Parser *parser, AstIndex *out);
internal b32 parse_cast(Parser *parser, AstIndex *out);
internal b32 parse_as(Parser *parser, AstIndex *out);
internal b32 parse_if_else(Parser *parser, AstIndex *out);
internal b32 parse_base_expression(Parser *parser, AstIndex *out);
internal b32 parse_expression(Parser *parser, AstIndex *out);
internal b32 parse_literal_int(Parser *parser, AstIndex *out);
internal b32 parse_literal_string(Parser *parser, AstIndex *out);
internal b32 parse_for(Parser *parser, AstIndex *out);
internal b32 parse_defer(Parser *parser, AstIndex *out);
internal b32 parse_const(Parser *parser, AstIndex *out);
internal b32 parse_identifier(Parser *parser, AstIndex *out);
internal b32 parse_param(Parser *parser, AstIndex *out);
internal b32 parse_builtin_print(Parser *parser, AstIndex *out);

internal b32 parse_expression_impl(Parser *parser, AstIndex *out, u32 prev_op);

internal b32 is_token_index_past_end(Tokens *tokens, TokenIndex idx) {
  return idx >= tokens->tok_count;
}

internal b32 is_parser_past_end(Parser *parser) {
  return is_token_index_past_end(parser->tokens, parser->at);
}

internal b32 next(Parser *parser, u8 *token_kind) {
  if (is_parser_past_end(parser)) {
    return False;
  }

  *token_kind = parser->tokens->kinds[parser->at];

  parser->at += 1;

  return True;
}

internal b32 expect_token(Parser *parser, u8 expected_token_kind) {
  u8 tok;
  b32 has_next = next(parser, &tok);

  if (!has_next) {
    Message_error(
      parser->msg_sink,
      (MessageLocation){ .kind = MessageLocation_end_of_file },
      string_lit("Expected a token, but encountered end of source.")
    );
    return False;
  }

  if (tok != expected_token_kind) {
    Message_error(
      parser->msg_sink,
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

  *token_kind = parser->tokens->kinds[parser->at];

  return True;
}

internal b32 peek_or_error(Parser *parser, u8 *token_kind) {
  b32 has_peeked = peek(parser, token_kind);
  if (!has_peeked) {
    Message_error(
      parser->msg_sink,
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

  *token_kind = parser->tokens->kinds[lookahead];

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

typedef b32 (*ParseItemFn)(Parser *parser, AstIndex *out);

internal b32 parse_comma_separated_items_until(
  Parser *parser, AstIndexList *items, ParseItemFn parse, u8 terminator
) {
  while (True) {
    consume_if_match(parser, Tok_comma);

    u8  tok;
    b32 has_next = peek(parser, &tok);

    if (!has_next || tok == terminator) {
      break;
    }

    AstIndex *item = list_push(items, parser->scratch);
    Try(parse(parser, item));
  }

  return True;
}

internal b32 parse_source(Parser *parser, AstIndex *out) {
  AstIndex idx = node_alloc(parser);
  TokenIndex start = parser->at;

  AstIndexList items = {0};

  while (!is_parser_past_end(parser)) {
    AstIndex *section = list_push(&items, parser->scratch);
    Try(parse_mod_section(parser, section));
  }

  AstSource *source = node_push_data_flex(parser, AstSource, items.len, idx);
  source->count = Cast(u32, items.len);
  list_copy_to_array(&items, source->items);

  *node_kind(parser, idx) = Ast_source;
  *node_span(parser, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = idx;

  return True;
}

internal b32 parse_mod_section(Parser *parser, AstIndex *out) {
  AstIndex idx = node_alloc(parser);
  TokenIndex start = parser->at;

  Try(expect_token(parser, Tok_keyword_mod));

  AstIndex name;
  Try(parse_identifier(parser, &name));

  AstIndexList items = {0};

  while (!is_parser_past_end(parser)) {
    u8 tok;
    peek(parser, &tok);
    if (tok == Tok_keyword_mod) {
      break;
    }

    AstIndex *decl = list_push(&items, parser->scratch);
    Try(parse_declaration(parser, decl));
  }

  AstModSection *section = node_push_data_flex(parser, AstModSection, items.len, idx);
  section->name  = name;
  section->count = Cast(u32, items.len);
  list_copy_to_array(&items, section->items);

  *node_kind(parser, idx) = Ast_mod_section;
  *node_span(parser, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = idx;

  return True;
}

internal b32 parse_builtin_print(Parser *parser, AstIndex *out) {
  AstIndex   idx   = node_alloc(parser);
  TokenIndex start = parser->at;

  Try(expect_token(parser, Tok_builtin_print));

  AstIndexList args = {0};

  Try(expect_token(parser, Tok_paren_open));
  Try(parse_comma_separated_items_until(parser, &args, parse_expression, Tok_paren_close));
  Try(expect_token(parser, Tok_paren_close));

  AstBuiltin *builtin = node_push_data_flex(parser, AstBuiltin, args.len, idx);
  builtin->kind  = Builtin_print;
  builtin->count = Cast(u32, args.len);
  list_copy_to_array(&args, builtin->args);

  *node_kind(parser, idx) = Ast_builtin;
  *node_span(parser, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = idx;

  return True;
}

internal b32 parse_block(Parser *parser, AstIndex *out) {
  AstIndex   idx   = node_alloc(parser);
  TokenIndex start = parser->at;

  AstIndexList items = {0};

  Try(expect_token(parser, Tok_brace_open));

  while (!is_parser_past_end(parser)) {
    u8 tok;
    Try(peek(parser, &tok));
    if (tok == Tok_brace_close) {
      break;
    }

    AstIndex *e = list_push(&items, parser->scratch);
    Try(parse_expression(parser, e));
  }

  Try(expect_token(parser, Tok_brace_close));

  AstBlock *block = node_push_data_flex(parser, AstBlock, items.len, idx);
  block->count = Cast(u32, items.len);
  list_copy_to_array(&items, block->items);

  *node_kind(parser, idx) = Ast_block;
  *node_span(parser, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = idx;

  return True;
}

internal b32 parse_type(Parser *parser, AstIndex *out) {
  AstIndex   idx   = node_alloc(parser);
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

      AstTypeSlice *type_slice = node_push_data(parser, AstTypeSlice, idx);
      Try(parse_type(parser, &type_slice->base));

      *node_kind(parser, idx) = Ast_type_slice;
    } else if (tok == Tok_literal_int) {
      AstTypeArray *type_array = node_push_data(parser, AstTypeArray, idx);
      Try(parse_literal_int(parser, &type_array->size));
      Try(expect_token(parser, Tok_bracket_close));
      Try(parse_type(parser, &type_array->base));

      *node_kind(parser, idx) = Ast_type_array;
    } else {
      // TODO: you could put any expression between the brackets really
      Panic();
    }
  } break;
  case Tok_paren_open: {
    u8 ignored;
    next(parser, &ignored);

    AstIndexList param_types = {0};
    Try(parse_comma_separated_items_until(parser, &param_types, parse_type, Tok_paren_close));
    Try(expect_token(parser, Tok_paren_close));

    AstIndex return_type;
    Try(parse_type(parser, &return_type));

    AstTypeFunction *type_function = node_push_data_flex(parser, AstTypeFunction, param_types.len, idx);
    type_function->return_type = return_type;
    type_function->count       = Cast(u32, param_types.len);
    list_copy_to_array(&param_types, type_function->param_types);

    *node_kind(parser, idx) = Ast_type_function;
  } break;
  case Tok_identifier: {
    TokenIndex identifier = parser->at;

    u8 ignored;
    next(parser, &ignored);

    TokenIndex *p = node_push_data(parser, TokenIndex, idx);
    *p = identifier;

    *node_kind(parser, idx) = Ast_identifier;
  } break;
  default:
    Message_error(
      parser->msg_sink,
      (MessageLocation){ .kind = MessageLocation_token_index, .data.token_index = parser->at },
      string_lit("Unexpected token {tok} encountered in type expression."), tok
    );
    return False;
  }

  *node_span(parser, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = idx;

  return True;
}

internal b32 parse_declaration(Parser *parser, AstIndex *out) {
  AstIndex   idx   = node_alloc(parser);
  TokenIndex start = parser->at;

  AstDeclaration *declaration = node_push_data(parser, AstDeclaration, idx);
  zero_struct(AstDeclaration, declaration);
  declaration->name = parser->at;

  Try(expect_token(parser, Tok_identifier));

  Try(expect_token(parser, Tok_colon));

  u8 tok;
  Try(peek_or_error(parser, &tok));

  if (tok != Tok_equals) {
    Try(parse_type(parser, &declaration->type));
  } else {
    declaration->type = 0;
  }

  Try(expect_token(parser, Tok_equals));

  Try(parse_expression(parser, &declaration->value));

  *node_kind(parser, idx) = Ast_declaration;
  *node_span(parser, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = idx;

  return True;
}

internal b32 parse_literal_int(Parser *parser, AstIndex *out) {
  AstIndex   idx   = node_alloc(parser);
  TokenIndex start = parser->at;

  TokenIndex literal_int = parser->at;

  Try(expect_token(parser, Tok_literal_int));

  TokenIndex *p = node_push_data(parser, TokenIndex, idx);
  *p = literal_int;

  *node_kind(parser, idx) = Ast_literal_int;
  *node_span(parser, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = idx;

  return True;
}

// NOTE: this function and the parse_literal_int function are basically the same.
internal b32 parse_literal_string(Parser *parser, AstIndex *out) {
  AstIndex   idx   = node_alloc(parser);
  TokenIndex start = parser->at;

  TokenIndex literal_string = parser->at;

  Try(expect_token(parser, Tok_literal_string));

  TokenIndex *p = node_push_data(parser, TokenIndex, idx);
  *p = literal_string;

  *node_kind(parser, idx) = Ast_literal_string;
  *node_span(parser, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = idx;

  return True;
}

internal b32 parse_param(Parser *parser, AstIndex *out) {
  AstIndex   idx   = node_alloc(parser);
  TokenIndex start = parser->at;

  AstParam *param = node_push_data(parser, AstParam, idx);
  zero_struct(AstParam, param);
  param->name = parser->at;

  Try(expect_token(parser, Tok_identifier));

  if (consume_if_match(parser, Tok_colon)) {
    Try(parse_type(parser, &param->type));
  }

  *node_kind(parser, idx) = Ast_param;
  *node_span(parser, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = idx;

  return True;
}

internal b32 parse_function(Parser *parser, AstIndex *out) {
  AstIndex   idx   = node_alloc(parser);
  TokenIndex start = parser->at;

  AstIndexList params = {0};

  Try(expect_token(parser, Tok_bar));
  Try(parse_comma_separated_items_until(parser, &params, parse_param, Tok_bar));
  Try(expect_token(parser, Tok_bar));

  AstIndex return_type = 0;
  if (consume_if_match(parser, Tok_colon)) {
    Try(parse_type(parser, &return_type));
  }

  AstIndex body;
  Try(parse_expression(parser, &body));

  AstFunction *function = node_push_data_flex(parser, AstFunction, params.len, idx);
  function->return_type = return_type;
  function->body        = body;
  function->count       = Cast(u32, params.len);
  list_copy_to_array(&params, function->params);

  *node_kind(parser, idx) = Ast_function;
  *node_span(parser, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = idx;

  return True;
}

internal b32 parse_for(Parser *parser, AstIndex *out) {
  AstIndex   idx   = node_alloc(parser);
  TokenIndex start = parser->at;

  Try(expect_token(parser, Tok_keyword_for));

  AstFor *for_ = node_push_data(parser, AstFor, idx);

  Try(parse_expression(parser, &for_->iterable));

  Try(expect_token(parser, Tok_keyword_do));

  Try(parse_identifier(parser, &for_->iterator));

  Try(parse_block(parser, &for_->body));

  *node_kind(parser, idx) = Ast_for;
  *node_span(parser, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = idx;

  return True;
}

internal b32 parse_defer(Parser *parser, AstIndex *out) {
  AstIndex   idx   = node_alloc(parser);
  TokenIndex start = parser->at;

  Try(expect_token(parser, Tok_keyword_defer));

  AstDefer *defer = node_push_data(parser, AstDefer, idx);
  Try(parse_expression(parser, &defer->value));

  *node_kind(parser, idx) = Ast_defer;
  *node_span(parser, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = idx;

  return True;
}

internal b32 parse_if_else(Parser *parser, AstIndex *out) {
  AstIndex   idx   = node_alloc(parser);
  TokenIndex start = parser->at;

  Try(expect_token(parser, Tok_keyword_if));

  AstIfElse *if_else = node_push_data(parser, AstIfElse, idx);

  Try(parse_expression(parser, &if_else->cond));

  Try(parse_block(parser, &if_else->then));

  u8 tok;
  Try(peek(parser, &tok));
  if (tok != Tok_keyword_else) {
    if_else->otherwise = (AstIndex){0};

    *node_kind(parser, idx) = Ast_if_else;
    *node_span(parser, idx) = (SpanToken){ .start = start, .end = parser->at, };

    *out = idx;

    return True;
  }

  u8 ignored;
  next(parser, &ignored);

  Try(parse_block(parser, &if_else->otherwise));

  *node_kind(parser, idx) = Ast_if_else;
  *node_span(parser, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = idx;

  return True;
}

internal b32 parse_const(Parser *parser, AstIndex *out) {
  AstIndex   idx   = node_alloc(parser);
  TokenIndex start = parser->at;

  Try(expect_token(parser, Tok_keyword_const));

  AstConst *const_ = node_push_data(parser, AstConst, idx);

  Try(parse_base_expression(parser, &const_->expr));

  *node_kind(parser, idx) = Ast_const;
  *node_span(parser, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = idx;

  return True;
}

internal b32 parse_cast(Parser *parser, AstIndex *out) {
  AstIndex   idx   = node_alloc(parser);
  TokenIndex start = parser->at;

  Try(expect_token(parser, Tok_keyword_cast));

  AstCast *cast = node_push_data(parser, AstCast, idx);

  Try(expect_token(parser, Tok_paren_open));
  Try(parse_type(parser, &cast->type_dst));
  Try(expect_token(parser, Tok_paren_close));

  Try(parse_base_expression(parser, &cast->value));

  *node_kind(parser, idx) = Ast_cast;
  *node_span(parser, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = idx;

  return True;
}

internal b32 parse_as(Parser *parser, AstIndex *out) {
  AstIndex   idx   = node_alloc(parser);
  TokenIndex start = parser->at;

  Try(expect_token(parser, Tok_keyword_as));

  AstCast *cast = node_push_data(parser, AstCast, idx);

  Try(expect_token(parser, Tok_paren_open));
  Try(parse_type(parser, &cast->type_dst));
  Try(expect_token(parser, Tok_paren_close));

  Try(parse_base_expression(parser, &cast->value));

  *node_kind(parser, idx) = Ast_as;
  *node_span(parser, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = idx;

  return True;
}

internal b32 parse_identifier(Parser *parser, AstIndex *out) {
  AstIndex   idx   = node_alloc(parser);
  TokenIndex start = parser->at;

  TokenIndex identifier = parser->at;

  Try(expect_token(parser, Tok_identifier));

  TokenIndex *p = node_push_data(parser, TokenIndex, idx);
  *p = identifier;

  *node_kind(parser, idx) = Ast_identifier;
  *node_span(parser, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = idx;

  return True;
}

internal b32 parse_base_expression(Parser *parser, AstIndex *out) {
  u8  tok;
  b32 has_next_token = peek(parser, &tok);
  if (!has_next_token) {
    Message_error(
      parser->msg_sink,
      (MessageLocation){ .kind = MessageLocation_end_of_file },
      string_lit("Unexpected end of source, while trying to parse base expression.")
    );
    return False;
  }

  AstIndex base;
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
      Message_error(
        parser->msg_sink,
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
    AstIndex   ast_index = node_alloc(parser);
    TokenIndex start     = parser->at;

    u8 ignored;
    next(parser, &ignored);

    AstUnaryOp *unary_op = node_push_data(parser, AstUnaryOp, ast_index);

    // clang-format off
    switch (tok) {
    case Tok_exclamation: unary_op->op_kind = Not;    break;
    case Tok_minus:       unary_op->op_kind = Negate; break;

    default: { Unreachable(); } break;
    }
    // clang-format on

    Try(parse_expression(parser, &unary_op->value));

    *node_kind(parser, ast_index) = Ast_unary_op;
    *node_span(parser, ast_index) = (SpanToken){ .start = start, .end = parser->at, };

    base = ast_index;
  } break;

  default:
    Message_error(
      parser->msg_sink,
      (MessageLocation){ .kind = MessageLocation_token_index, .data.token_index = parser->at },
      string_lit("Unexpected token {tok} encountered at start of expression."), tok
    );
    return False;
  }

  while (!is_parser_past_end(parser)) {
    u8 tok;
    peek(parser, &tok);

    if (tok == Tok_bracket_open) {
      AstIndex   ast_index = node_alloc(parser);
      TokenIndex start     = parser->at;

      u8 ignored;
      next(parser, &ignored);

      AstIndexData *index = node_push_data(parser, AstIndexData, ast_index);
      index->indexable = base;
      Try(parse_expression(parser, &index->index_at));

      Try(expect_token(parser, Tok_bracket_close));

      *node_kind(parser, ast_index) = Ast_index;
      *node_span(parser, ast_index) = (SpanToken){ .start = start, .end = parser->at, };

      base = ast_index;

      continue;
    }

    if (tok == Tok_paren_open) {
      AstIndex   ast_index = node_alloc(parser);
      TokenIndex start     = parser->at;

      u8 ignored;
      next(parser, &ignored);

      AstIndexList args = {0};
      Try(parse_comma_separated_items_until(parser, &args, parse_expression, Tok_paren_close));
      Try(expect_token(parser, Tok_paren_close));

      AstCall *call = node_push_data_flex(parser, AstCall, args.len, ast_index);
      call->callee = base;
      call->count  = Cast(u32, args.len);
      list_copy_to_array(&args, call->args);

      *node_kind(parser, ast_index) = Ast_call;
      *node_span(parser, ast_index) = (SpanToken){ .start = start, .end = parser->at, };

      base = ast_index;

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

internal b32 parse_expression_impl(Parser *parser, AstIndex *out, u32 prev_op) {
  AstIndex lhs;
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

    AstIndex   ast_index = node_alloc(parser);
    TokenIndex start     = parser->at;

    u8 ignored;
    next(parser, &ignored);

    AstIndex rhs;
    Try(parse_expression_impl(parser, &rhs, op));

    if (op >= BinaryOpKind_max) {
      AstAssign *assign = node_push_data(parser, AstAssign, ast_index);
      assign->assign_kind = Assign_normal;
      assign->lhs         = lhs;
      assign->value       = rhs;

      *node_kind(parser, ast_index) = Ast_assign;
      *node_span(parser, ast_index) = (SpanToken){ .start = start, .end = parser->at, };
    } else {
      AstBinaryOp *binary_op = node_push_data(parser, AstBinaryOp, ast_index);
      binary_op->op_kind = op;
      binary_op->lhs     = lhs;
      binary_op->rhs     = rhs;

      *node_kind(parser, ast_index) = Ast_binary_op;
      *node_span(parser, ast_index) = (SpanToken){ .start = start, .end = parser->at, };
    }

    lhs = ast_index;
  }

  *out = lhs;

  return True;
}

internal b32 parse_expression(Parser *parser, AstIndex *out) {
  return parse_expression_impl(parser, out, Op_count);
}

void *ast_data(AstNodes2 *ast, AstIndex idx) {
  return ptr_offset(ast->extra, ast->datas[idx]);
}

b32 parse(ParseContext *context, Tokens *tokens, AstNodes2 *ast) {
  ArenaSnapshot scope = arena_scope_begin(context->scratch);

  Parser parser = {
    .tokens   = tokens,
    .at       = 0,
    .msg_sink = context->msg_sink,
    .arena    = context->arena,
    .scratch  = context->scratch,
  };

  // Reserve index 0 so that 0 can be used as 'no node'. The root is at index 1.
  node_alloc(&parser);

  AstIndex root;
  b32 ok = parse_source(&parser, &root);

  u32 count = Cast(u32, parser.kinds.len);

  u8        *kinds = arena_push_array(u8,        context->arena, count);
  SpanToken *spans = arena_push_array(SpanToken, context->arena, count);
  u32       *datas = arena_push_array(u32,       context->arena, count);

  kindlist_copy_to_array(&parser.kinds, kinds);
  spanlist_copy_to_array(&parser.spans, spans);
  datalist_copy_to_array(&parser.datas, datas);

  *ast = (AstNodes2){
    .count = count,
    .kinds = kinds,
    .spans = spans,
    .datas = datas,
    .extra = context->arena->base,
  };

  arena_scope_end(context->scratch, scope);

  return ok;
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
