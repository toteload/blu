#include "blu.h"
#include "ast.h"
#include "tokens.h"
#include "messages.h"

#define SEGMENTLIST_NAME            NodeIndexList
#define SEGMENTLIST_TYPE            NodeIndex
#define SEGMENTLIST_FUNCTION_PREFIX list
#define SEGMENTLIST_MIN_SIZE_LOG2   3
#define SEGMENTLIST_SEGMENT_COUNT   24
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

#define Try(e)                                                                                     \
  if (!(e)) {                                                                                      \
    return False;                                                                                  \
  }

typedef struct {
  Messages *messages;
  Tokens *tokens;
  AstNodes *nodes;
  TokenIndex at;
} Parser;

internal b32 parse_root(Parser *parser, NodeIndex *out);
internal b32 parse_declaration(Parser *parser, NodeIndex *out);
internal b32 parse_block(Parser *parser, NodeIndex *out);
internal b32 parse_type(Parser *parser, NodeIndex *out);
internal b32 parse_function(Parser *parser, NodeIndex *out);
internal b32 parse_cast(Parser *parser, NodeIndex *out);
internal b32 parse_if_else(Parser *parser, NodeIndex *out);
internal b32 parse_base_expression(Parser *parser, NodeIndex *out);
internal b32 parse_expression(Parser *parser, NodeIndex *out);

internal b32 is_token_index_past_end(Tokens *tokens, TokenIndex idx) {
  return idx >= tokens_count(tokens);
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
    messages_add_error(parser->messages, string_lit("Expected a token, but encountered end of source."));
    return False;
  }

  if (tok != expected_token_kind) {
    messages_add_error(parser->messages, string_lit("Expected a certain token, but got another."));
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

internal void *nodes_push_data_raw(AstNodes *nodes, AstIndex idx, usize size, u32 align) {
  void *p = arena_push(&nodes->extra, size, align);
  u32 offset = Cast(u32, ptr_diff(p, nodes->extra.base));
  Cast(u32*, nodes->datas.base)[idx] = offset;
  return p;
}

#define nodes_push_data(nodes, type, idx) nodes_push_data_raw(nodes, idx, sizeof(type), Align_of(type))

internal b32 parse_root(Parser *parser, NodeIndex *out) {
  AstIndex idx = nodes_alloc(parser->nodes);
  TokenIndex start = parser->at;

  AstRoot *root = nodes_push_data(parser->nodes, AstRoot, idx);
  zero_struct(AstRoot, root);

  while (!is_parser_past_end(parser)) {
    NodeIndex *decl = list_push(&root->items, &parser->nodes->extra);
    Try(parse_declaration(parser, decl));
  }

  *nodes_kind(parser->nodes, idx) = Ast_root;
  *nodes_span(parser->nodes, idx) = (SpanToken){ .start = start, .end = parser->at, };

  *out = (NodeIndex){ .kind = NodeIndex_ast, .idx.ast = idx, };

  return True;
}
