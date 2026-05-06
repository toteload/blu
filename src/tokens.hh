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

  Tok_keyword_if,
  Tok_keyword_else,
  Tok_keyword_while,
  Tok_keyword_break,
  Tok_keyword_continue,
  Tok_keyword_return,
  Tok_keyword_and,
  Tok_keyword_or,

  Tok_identifier,
  Tok_builtin,

  Tok_line_comment,

  Tok_kind_max,
};

// TODO: use Index type.
struct TokenIndex {
  u32 idx;
};

struct Tokens {
  Vector<TokenKind> kinds;

  // Each span denotes what bytes in the source the token spans.
  Vector<Span<u32>> spans;

  u32 len() { return kinds.len(); }
  TokenIndex end() { return {cast<u32>(kinds.len())}; }

  TokenIndex alloc() {
    TokenIndex res = end();
    kinds.push_empty();
    spans.push_empty();
    return res;
  }

  TokenKind &kind(TokenIndex idx) { return kinds[idx.idx]; }
  Span<u32> &span(TokenIndex idx) { return spans[idx.idx]; }
};

constexpr char const *token_string[Tok_kind_max] = {
  "colon",          "semicolon",     "comma",        "dot",        "equals",
  "minus",          "plus",          "star",         "slash",      "percent",
  "plus_equals",    "exclamation",   "ampersand",    "bar",        "caret",
  "tilde",          "left-shift",    "right-shift",  "cmp-eq",     "cmp-ne",
  "cmp-gt",         "cmp-ge",        "cmp-lt",       "cmp-le",     "literal-int",
  "literal-string", "brace-open",    "brace-close",  "paren-open", "paren-close",
  "bracket-open",   "bracket-close", "if",           "else",       "while",
  "break",          "continue",      "return",       "and",        "or",
  "identifier",     "builtin",       "line-comment",
};

ttld_inline char const *token_kind_string(u32 kind) {
  if (kind >= Tok_kind_max) {
    return "<illegal>";
  }

  return token_string[kind];
}
