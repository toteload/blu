#include "blu.h"
#include "ast.h"
#include "tokens.h"
#include "source_file.h"

#define Binary_and_assign_op_count (BinaryOpKind_count + AssignKind_count)

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

typedef void *VoidPtr;

#define SEGMENTLIST_NAME            TmpList
#define SEGMENTLIST_TYPE            VoidPtr
#define SEGMENTLIST_FUNCTION_PREFIX tmplist
#define SEGMENTLIST_LINKAGE         internal
#define SEGMENTLIST_MIN_SIZE_LOG2   8
#define SEGMENTLIST_SEGMENT_COUNT   24
#define SEGMENTLIST_OUTPUT_TYPES
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wflexible-array-extensions"

typedef struct {
  AstIndexList items;
  AstSource    base;
} AstSourceTmp;

typedef struct {
  AstIndexList  items;
  AstModSection base;
} AstModSectionTmp;

typedef struct {
  AstIndexList items;
  AstBlock     base;
} AstBlockTmp;

typedef struct {
  AstIndexList args;
  AstBuiltin   base;
} AstBuiltinTmp;

typedef struct {
  AstIndexList    param_types;
  AstTypeFunction base;
} AstTypeFunctionTmp;

typedef struct {
  AstIndexList params;
  AstFunction  base;
} AstFunctionTmp;

typedef struct {
  AstIndexList args;
  AstCall      base;
} AstCallTmp;

#pragma clang diagnostic pop

_Static_assert(Offsetof(AstSourceTmp, items)             == 0, "List must be at offset 0.");
_Static_assert(Offsetof(AstModSectionTmp, items)         == 0, "List must be at offset 0.");
_Static_assert(Offsetof(AstBlockTmp, items)              == 0, "List must be at offset 0.");
_Static_assert(Offsetof(AstBuiltinTmp, args)             == 0, "List must be at offset 0.");
_Static_assert(Offsetof(AstTypeFunctionTmp, param_types) == 0, "List must be at offset 0.");
_Static_assert(Offsetof(AstFunctionTmp, params)          == 0, "List must be at offset 0.");
_Static_assert(Offsetof(AstCallTmp, args)                == 0, "List must be at offset 0.");

#define Tmp_base_offset sizeof(AstIndexList)

_Static_assert(Offsetof(AstSourceTmp, base)       == Tmp_base_offset, "Base must be at a common offset.");
_Static_assert(Offsetof(AstModSectionTmp, base)   == Tmp_base_offset, "Base must be at a common offset.");
_Static_assert(Offsetof(AstBlockTmp, base)        == Tmp_base_offset, "Base must be at a common offset.");
_Static_assert(Offsetof(AstBuiltinTmp, base)      == Tmp_base_offset, "Base must be at a common offset.");
_Static_assert(Offsetof(AstTypeFunctionTmp, base) == Tmp_base_offset, "Base must be at a common offset.");
_Static_assert(Offsetof(AstFunctionTmp, base)     == Tmp_base_offset, "Base must be at a common offset.");

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
  TmpList      tmp;

  Arena *arena;
  Arena *scratch;
} Parser;

internal AstIndex node_alloc(Parser *parser) {
  AstIndex idx = parser->kinds.len;
  kindlist_append(&parser->kinds, parser->scratch, 0);
  spanlist_append(&parser->spans, parser->scratch, (SpanToken){0});
  tmplist_append(&parser->tmp, parser->scratch, Null);
  return idx;
}

internal u8 *node_kind(Parser *parser, AstIndex idx) {
  return kindlist_ptr_at_unchecked(&parser->kinds, idx);
}

internal SpanToken *node_span(Parser *parser, AstIndex idx) {
  return spanlist_ptr_at_unchecked(&parser->spans, idx);
}

internal void *node_push_data_raw(Parser *parser, AstIndex idx, usize size, u32 align) {
  void *p = arena_push(parser->scratch, size, align);
  *tmplist_ptr_at_unchecked(&parser->tmp, idx) = p;
  return p;
}

#define node_push_data(parser, type, idx) node_push_data_raw(parser, idx, sizeof(type), Align_of(type))

internal b32 parse_source(Parser *parser, AstIndex *out);
internal b32 parse_mod_section(Parser *parser, AstIndex *out);
internal b32 parse_declaration(Parser *parser, AstIndex *out);
internal b32 parse_block(Parser *parser, AstIndex *out);
internal b32 parse_type(Parser *parser, AstIndex *out);
internal b32 parse_function(Parser *parser, AstIndex *out);
internal b32 parse_cast(Parser *parser, AstIndex *out);
internal b32 parse_as(Parser *parser, AstIndex *out);
internal b32 parse_break(Parser *parser, AstIndex *out);
internal b32 parse_label(Parser *parser, AstIndex *out);
internal b32 parse_if_else(Parser *parser, AstIndex *out);
internal b32 parse_base_expression(Parser *parser, AstIndex *out);
internal b32 parse_expression(Parser *parser, AstIndex *out);
internal b32 parse_literal_int(Parser *parser, AstIndex *out);
internal b32 parse_literal_string(Parser *parser, AstIndex *out);
internal b32 parse_for(Parser *parser, AstIndex label, AstIndex *out);
internal b32 parse_while(Parser *parser, AstIndex label, AstIndex *out);
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
      (MessageLocation){ 
        .kind = MessageLocation_byte_offset,
        .data.offset = parser->tokens->lines[parser->tokens->line_count-1],
      },
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
      (MessageLocation){ 
        .kind = MessageLocation_byte_offset,
        .data.offset = parser->tokens->lines[parser->tokens->line_count-1],
      },
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

  AstSourceTmp *source = node_push_data(parser, AstSourceTmp, idx);
  zero_struct(AstSourceTmp, source);

  while (!is_parser_past_end(parser)) {
    AstIndex *section = list_push(&source->items, parser->scratch);
    Try(parse_mod_section(parser, section));
  }

  *node_kind(parser, idx) = Ast_source;
  *node_span(parser, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = idx;

  return True;
}

internal b32 parse_mod_section(Parser *parser, AstIndex *out) {
  AstIndex idx = node_alloc(parser);
  TokenIndex start = parser->at;

  Try(expect_token(parser, Tok_keyword_mod));

  AstModSectionTmp *section = node_push_data(parser, AstModSectionTmp, idx);
  zero_struct(AstModSectionTmp, section);

  TokenIndex name = parser->at;

  Try(expect_token(parser, Tok_identifier));

  section->base.name = name;

  while (!is_parser_past_end(parser)) {
    u8 tok;
    peek(parser, &tok);
    if (tok == Tok_keyword_mod) {
      break;
    }

    AstIndex *decl = list_push(&section->items, parser->scratch);
    Try(parse_declaration(parser, decl));
  }

  *node_kind(parser, idx) = Ast_mod_section;
  *node_span(parser, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = idx;

  return True;
}

internal b32 parse_builtin_debug(Parser *parser, AstIndex *out) {
  AstIndex   idx   = node_alloc(parser);
  TokenIndex start = parser->at;

  Try(expect_token(parser, Tok_builtin_debug));

  AstBuiltinTmp *builtin = node_push_data(parser, AstBuiltinTmp, idx);
  zero_struct(AstBuiltinTmp, builtin);
  builtin->base.kind = Builtin_debug;

  Try(expect_token(parser, Tok_paren_open));
  AstIndex *e = list_push(&builtin->args, parser->scratch);
  Try(parse_expression(parser, e));
  Try(expect_token(parser, Tok_paren_close));

  *node_kind(parser, idx) = Ast_builtin;
  *node_span(parser, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = idx;

  return True;
}

internal b32 parse_block(Parser *parser, AstIndex *out) {
  AstIndex   idx   = node_alloc(parser);
  TokenIndex start = parser->at;

  AstBlockTmp *block = node_push_data(parser, AstBlockTmp, idx);
  zero_struct(AstBlockTmp, block);

  Try(expect_token(parser, Tok_brace_open));

  while (!is_parser_past_end(parser)) {
    u8 tok;
    Try(peek(parser, &tok));
    if (tok == Tok_brace_close) {
      break;
    }

    AstIndex *e = list_push(&block->items, parser->scratch);
    Try(parse_expression(parser, e));
  }

  Try(expect_token(parser, Tok_brace_close));

  *node_kind(parser, idx) = Ast_block;
  *node_span(parser, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = idx;

  return True;
}

internal b32 parse_type(Parser *parser, AstIndex *out) {
  u8 tok;
  Try(peek(parser, &tok));

  if (tok == Tok_brace_open) {
    u8 ignored;
    next(parser, &ignored);

    Try(parse_type(parser, out));

    Try(expect_token(parser, Tok_brace_close));

    return True;
  }

  AstIndex   idx   = node_alloc(parser);
  TokenIndex start = parser->at;

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

    AstTypeFunctionTmp *type_function = node_push_data(parser, AstTypeFunctionTmp, idx);
    zero_struct(AstTypeFunctionTmp, type_function);

    Try(parse_comma_separated_items_until(parser, &type_function->param_types, parse_type, Tok_paren_close));
    Try(expect_token(parser, Tok_paren_close));

    Try(parse_type(parser, &type_function->base.return_type));

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

  AstFunctionTmp *function = node_push_data(parser, AstFunctionTmp, idx);
  zero_struct(AstFunctionTmp, function);

  Try(expect_token(parser, Tok_bar));
  Try(parse_comma_separated_items_until(parser, &function->params, parse_param, Tok_bar));
  Try(expect_token(parser, Tok_bar));

  if (consume_if_match(parser, Tok_colon)) {
    Try(parse_type(parser, &function->base.return_type));
  }

  Try(parse_expression(parser, &function->base.body));

  *node_kind(parser, idx) = Ast_function;
  *node_span(parser, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = idx;

  return True;
}

internal b32 parse_for(Parser *parser, AstIndex label, AstIndex *out) {
  AstIndex   idx   = node_alloc(parser);
  TokenIndex start = parser->at;

  Try(expect_token(parser, Tok_keyword_for));

  AstFor *for_ = node_push_data(parser, AstFor, idx);

  for_->label = label;

  Try(parse_expression(parser, &for_->iterable));

  Try(expect_token(parser, Tok_keyword_do));

  Try(parse_identifier(parser, &for_->iterator));

  Try(parse_block(parser, &for_->body));

  *node_kind(parser, idx) = Ast_for;
  *node_span(parser, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = idx;

  return True;
}

internal b32 parse_while(Parser *parser, AstIndex label, AstIndex *out) {
  AstIndex   idx   = node_alloc(parser);
  TokenIndex start = parser->at;

  Try(expect_token(parser, Tok_keyword_while));

  AstWhile *data = node_push_data(parser, AstWhile, idx);

  data->label = label;

  Try(parse_expression(parser, &data->cond));

  Try(parse_block(parser, &data->body));

  *node_kind(parser, idx) = Ast_while;
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

  b32 has_do = consume_if_match(parser, Tok_keyword_do);

  if (has_do) {
    Try(parse_expression(parser, &if_else->then));
  } else {
    Try(parse_block(parser, &if_else->then));
  }

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

  if (has_do) {
    Try(parse_expression(parser, &if_else->otherwise));
  } else {
    Try(parse_block(parser, &if_else->otherwise));
  }

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

internal b32 parse_break(Parser *parser, AstIndex *out) {
  AstIndex   idx   = node_alloc(parser);
  TokenIndex start = parser->at;

  Try(expect_token(parser, Tok_keyword_break));

  AstBreak *data = node_push_data(parser, AstBreak, idx);
  *data = (AstBreak){ 0 };

  u8 tok;
  Try(peek(parser, &tok));
  if (tok == Tok_label) {
    Try(parse_label(parser, &data->label));
  }

  Try(peek(parser, &tok));
  if (tok != Tok_brace_close) {
    Try(parse_expression(parser, &data->value));
  }

  *node_kind(parser, idx) = Ast_break;
  *node_span(parser, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = idx;

  return True;
}

internal b32 parse_label(Parser *parser, AstIndex *out) {
  AstIndex   idx   = node_alloc(parser);
  TokenIndex start = parser->at;

  TokenIndex label = parser->at;

  Try(expect_token(parser, Tok_label));

  TokenIndex *p = node_push_data(parser, TokenIndex, idx);
  *p = label;

  *node_kind(parser, idx) = Ast_label;
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
      (MessageLocation){ 
        .kind = MessageLocation_byte_offset,
        .data.offset = parser->tokens->lines[parser->tokens->line_count-1],
      },
      string_lit("Unexpected end of source, while trying to parse base expression.")
    );
    return False;
  }

  AstIndex base;
  switch (tok) {
    // clang-format off
  case Tok_keyword_for:    Try(parse_for(parser, 0, &base));         break;
  case Tok_keyword_while:  Try(parse_while(parser, 0, &base));       break;
  case Tok_keyword_defer:  Try(parse_defer(parser, &base));          break;
  case Tok_keyword_if:     Try(parse_if_else(parser, &base));        break;
  case Tok_literal_int:    Try(parse_literal_int(parser, &base));    break;
  case Tok_literal_string: Try(parse_literal_string(parser, &base)); break;
  case Tok_bar:            Try(parse_function(parser, &base));       break;
  case Tok_builtin_debug:  Try(parse_builtin_debug(parser, &base));  break;
  case Tok_keyword_const:  Try(parse_const(parser, &base));          break;
  case Tok_keyword_cast:   Try(parse_cast(parser, &base));           break;
  case Tok_keyword_as:     Try(parse_as(parser, &base));             break;
  case Tok_keyword_break:  Try(parse_break(parser, &base));          break;
  case Tok_brace_open:     Try(parse_block(parser, &base));          break;
    // clang-format on

  case Tok_label: { 
    AstIndex label;
    Try(parse_label(parser, &label));

    Try(peek(parser, &tok));
    switch (tok) {
      // clang-format off
    case Tok_keyword_for:   Try(parse_for(parser, label, &base));   break;
    case Tok_keyword_while: Try(parse_while(parser, label, &base)); break;
      // clang-format on
    default: Todo();
    }
  } break;

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

      AstCallTmp *call = node_push_data(parser, AstCallTmp, ast_index);
      call->base.callee = base;
      call->args        = (AstIndexList){0};
      Try(parse_comma_separated_items_until(parser, &call->args, parse_expression, Tok_paren_close));
      Try(expect_token(parser, Tok_paren_close));

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
internal const u8 op_precedence_group[Binary_and_assign_op_count] = {
  10, 10, 10,
  20, 20,
  30, 30,
  40, 40, 40,
  50, 50, 50, 50, 50, 50,
  60, 60,
  100, 100, 100, 100,
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

    u8 op = Binary_and_assign_op_count;

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

    case Tok_equals:         op = BinaryOpKind_count + Assign_normal; break;
    case Tok_minus_equals:   op = BinaryOpKind_count + Assign_sub;    break;
    case Tok_plus_equals:    op = BinaryOpKind_count + Assign_add;    break;
    case Tok_star_equals:    op = BinaryOpKind_count + Assign_mul;    break;
    case Tok_percent_equals: op = BinaryOpKind_count + Assign_mod;    break;

    default: { *out = lhs; return True; }
    }
    // clang-format on

    enum Precedence precedence = Prec_right;
    if (prev_op != Binary_and_assign_op_count) {
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

    if (op >= BinaryOpKind_count) {
      AstAssign *assign = node_push_data(parser, AstAssign, ast_index);

      assign->kind  = op - BinaryOpKind_count;
      assign->lhs   = lhs;
      assign->value = rhs;

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
  return parse_expression_impl(parser, out, Binary_and_assign_op_count);
}

void *ast_data(AstNodes *ast, AstIndex idx) {
  return ptr_offset(ast->extra, ast->datas[idx]);
}

internal b32 has_variable_length_payload(u8 kind) {
  switch (kind) {
  case Ast_source:
  case Ast_mod_section:
  case Ast_block:
  case Ast_type_function:
  case Ast_builtin:
  case Ast_call:
  case Ast_function:
    return True;
  default:
    return False;
  }
}

internal u32 base_payload_size(u8 kind) {
  switch (Cast(AstKind, kind)) {
#define X(k,d,s) case k: return sizeof(d);
#include "x_ast_kinds.h"
#undef X
  }

  Unreachable();
}

internal u32 payload_align(u8 kind) {
  switch (Cast(AstKind, kind)) {
#define X(k,d,s) case k: return Align_of(d);
#include "x_ast_kinds.h"
#undef X
  }

  Unreachable();
}

internal void *push_data(void *base, Arena *extra, u32 *datas, AstIndex i, u32 size, u32 align) {
  void *p = arena_push(extra, size, align);
  u32 offset = ptr_diff(p, base);
  datas[i] = offset;
  return p;
}

b32 parse(ParseContext *context, Tokens *tokens, AstNodes *ast) {
  ArenaSnapshot scope = arena_scope_begin(context->scratch);

  Parser parser = {
    .tokens   = tokens,
    .at       = 0,
    .msg_sink = context->msg_sink,
    .arena    = context->arena,
    .scratch  = context->scratch,
  };

  // Reserve the zero index
  node_alloc(&parser);

  AstIndex root;
  b32 ok = parse_source(&parser, &root);

  if (!ok) {
    arena_scope_end(context->scratch, scope);
    return False;
  }

  u32 count = Cast(u32, parser.kinds.len);

  u8        *kinds = arena_push_array(u8,        context->arena, count);
  SpanToken *spans = arena_push_array(SpanToken, context->arena, count);
  u32       *datas = arena_push_array(u32,       context->arena, count);

  kindlist_copy_to_array(&parser.kinds, kinds);
  spanlist_copy_to_array(&parser.spans, spans);

  void *extra = context->arena->at;

  for (AstIndex i = 1; i < count; i++) {
    u8 kind = kinds[i];
    u32 size = base_payload_size(kind);
    u32 align = payload_align(kind);
    void *payload = tmplist_at_unchecked(&parser.tmp, i);

    if (!has_variable_length_payload(kind)) {
      void *mem = push_data(extra, context->arena, datas, i, size, align);
      memcpy(mem, payload, size);
    } else {
      AstIndexList *list = payload;
      u32 n = list->len;
      void *mem = push_data(extra, context->arena, datas, i, size + n * sizeof(AstIndex), align);
      void *base = ptr_offset(payload, Tmp_base_offset);
      memcpy(mem, base, size);
      // TODO: change the way the count pointer is computed, because this feels a bit fragile.
      // It is assumed here that the count is a u32, and that it is the last 4 bytes of the base payload.
      // These assumptions are not checked/enforced anywhere.
      u32 *count = Cast(u32*, ptr_offset(mem, size - sizeof(u32)));
      *count = n;
      list_copy_to_array(list, ptr_offset(mem, size));
    }
  }

  *ast = (AstNodes){
    .count = count,
    .kinds = kinds,
    .spans = spans,
    .datas = datas,
    .extra = extra,
  };

  arena_scope_end(context->scratch, scope);

  return ok;
}

String ast_string[] = {
#define X(k,d,s) [k] = string_lit(s),
#include "x_ast_kinds.h"
#undef X
};

String ast_kind_string(u8 kind) {
  if (kind >= Ast_kind_max) {
    return string_lit("<illegal-ast-kind>");
  }

  return ast_string[kind];
}
