#ifndef TOKENS_H
#define TOKENS_H

#include "toteload.h"

enum TokenKind {
  Tok_colon,
  Tok_semicolon,
  Tok_comma,
  Tok_dot,
  Tok_arrow,

  Tok_equals,
  Tok_minus,
  Tok_plus,
  Tok_star,
  Tok_slash,
  Tok_percent,
  Tok_plus_equals,
  Tok_exclamation,
  Tok_ampersand,
  Tok_bar,
  Tok_caret,
  Tok_tilde,
  Tok_left_shift,
  Tok_right_shift,
  Tok_cmp_eq,
  Tok_cmp_ne,
  Tok_cmp_gt,
  Tok_cmp_ge,
  Tok_cmp_lt,
  Tok_cmp_le,

  Tok_literal_int,
  Tok_literal_string,

  Tok_brace_open,
  Tok_brace_close,
  Tok_paren_open,
  Tok_paren_close,
  Tok_bracket_open,
  Tok_bracket_close,

  Tok_keyword_if,
  Tok_keyword_else,
  Tok_keyword_for,
  Tok_keyword_do,
  Tok_keyword_break,
  Tok_keyword_continue,
  Tok_keyword_return,
  Tok_keyword_and,
  Tok_keyword_or,
  Tok_keyword_defer,
  Tok_keyword_const,
  Tok_keyword_cast,
  Tok_keyword_bitcast,
  Tok_keyword_as,
  Tok_keyword_mod,

  Tok_keyword_no_cache,
  Tok_keyword_inline,

  Tok_identifier,

  Tok_builtin_print,

  Tok_line_comment,
  Tok_newline,

  Tok_kind_max,
};

typedef struct {
  u32 start;
  u32 end;
} SpanU32;

#define KindList_min_size_log2    6
#define KindList_segment_count    24
#define SEGMENTLIST_NAME          KindList
#define SEGMENTLIST_TYPE          u8
#define SEGMENTLIST_MIN_SIZE_LOG2 KindList_min_size_log2
#define SEGMENTLIST_SEGMENT_COUNT KindList_segment_count
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

#define SpanList_min_size_log2    6
#define SpanList_segment_count    24
#define SEGMENTLIST_NAME          SpanList
#define SEGMENTLIST_TYPE          SpanU32
#define SEGMENTLIST_MIN_SIZE_LOG2 SpanList_min_size_log2
#define SEGMENTLIST_SEGMENT_COUNT SpanList_segment_count
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

#define OffsetList_min_size_log2  6
#define OffsetList_segment_count  24
#define SEGMENTLIST_NAME          OffsetList
#define SEGMENTLIST_TYPE          u32
#define SEGMENTLIST_MIN_SIZE_LOG2 OffsetList_min_size_log2
#define SEGMENTLIST_SEGMENT_COUNT OffsetList_segment_count
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

typedef struct {
  KindList    kinds;
  SpanList    spans;
  OffsetList  lines;
} Tokens;

typedef struct {
  u32 line;
  u32 offset_start_of_line;
  u32 line_len;
} LineInfo;

void       tokens_init(Tokens *tokens, Arena *arena);

u32        tokens_count(Tokens *tokens);

TokenIndex tokens_begin(Tokens *tokens);
TokenIndex tokens_end(Tokens *tokens);

LineInfo   tokens_find_line_info(Tokens *tokens, u32 byte_offset);

TokenIndex tokens_alloc(Tokens *tokens, Arena *arena);

u8        *tokens_kind(Tokens *tokens, TokenIndex idx);
SpanU32   *tokens_span(Tokens *tokens, TokenIndex idx);

char const *token_kind_string(u8 kind);

#endif // TOKENS_H
