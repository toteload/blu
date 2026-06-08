#ifndef TOKENS_H
#define TOKENS_H

#include "toteload.h"

enum TokenKind {
  Tok_colon,
  Tok_semicolon,
  Tok_comma,
  Tok_dot,

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

  Tok_identifier,

  Tok_builtin_print,

  Tok_line_comment,

  Tok_kind_max,
};

typedef struct {
  u32 start;
  u32 end;
} SpanU32;

typedef struct {
  Arena kinds;
  Arena spans;
} Tokens;

#define Max_tokens ((usize)1 << 24)

void tokens_init(Tokens *tokens);
void tokens_deinit(Tokens *tokens);
u32  tokens_count(Tokens *tokens);
TokenIndex tokens_alloc(Tokens *tokens);

#endif // TOKENS_H
