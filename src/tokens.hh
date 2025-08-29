enum TokenKind : u8 {
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

  Tok_keyword_fn,

  Tok_keyword_distinct,

  Tok_keyword_if,
  Tok_keyword_else,
  Tok_keyword_while,
  Tok_keyword_break,
  Tok_keyword_continue,
  Tok_keyword_return,
  Tok_keyword_and,
  Tok_keyword_or,



  Tok_identifier,

  Tok_line_comment,

  Tok_kind_max,
};

constexpr char const *token_string[Tok_kind_max + 1] = {
  "colon",        "semicolon",    "comma", "dot", "equals",      "minus",       "plus",       "star",
  "slash",        "percent",        "plus_equals", "exclamation", "ampersand",  "bar",
  "caret",        "tilde",          "left-shift",  "right-shift", "cmp-eq",     "cmp-ne",
  "cmp-gt",       "cmp-ge",         "cmp-lt",      "cmp-le",
  "literal_int",  "literal_string", "brace_open",  "brace_close", "paren_open", "paren_close",
  "bracket_open", "bracket_close",  "fn",          "distinct",      "if",         "else",
  "while",        "break",          "continue", "return",    "and",         "or",        
      "identifier", 
  "line_comment", "<illegal>",
};

ttld_inline char const *token_kind_string(u32 kind) {
  if (kind >= Tok_kind_max) {
    return token_string[Tok_kind_max];
  }

  return token_string[kind];
}
