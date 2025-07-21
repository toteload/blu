#pragma once

#include "toteload.hh"

// -[ Source location ]-

struct SourceLocation {
  u32 line      = 0;
  u32 col       = 0;
};

struct SourceSpan {
  SourceLocation start;
  SourceLocation end;

  static SourceSpan from_single_location(SourceLocation loc) {
    return {
      loc,
      {
        loc.line,
        loc.col + 1,
      }
    };
  }
};

// -[ Token ]-

enum TokenKind : u32 {
  Tok_colon,
  Tok_arrow,
  Tok_semicolon,
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
  Tok_comma,
  Tok_dot,
  Tok_literal_int,
  Tok_literal_string,
  Tok_brace_open,
  Tok_brace_close,
  Tok_paren_open,
  Tok_paren_close,
  Tok_bracket_open,
  Tok_bracket_close,
  Tok_keyword_fn,
  Tok_keyword_return,
  Tok_keyword_if,
  Tok_keyword_else,
  Tok_keyword_while,
  Tok_keyword_break,
  Tok_keyword_continue,
  Tok_keyword_and,
  Tok_keyword_or,
  Tok_keyword_for,
  Tok_keyword_in,
  Tok_keyword_cast,
  Tok_keyword_module,
  Tok_identifier,
  Tok_builtin,
  Tok_line_comment,
  Tok_kind_max,
};

struct Token {
  TokenKind kind;
  SourceSpan span;
  Str str;
};

enum TokenizerResult : u32 {
  TokResult_ok,
  TokResult_end,
  TokResult_unrecognized_token,
};

// Arena is only used to allocate memory for potential messages.
b32 tokenize(Str source, Vector<Token> *out, Arena *arena, Vector<Message> *messages);

