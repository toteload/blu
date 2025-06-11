#include "blu.hh"

struct Tokenizer {
  CompilerContext *ctx;
  char const *at;
  char const *end;

  SourceLocation current_location;

  void init(CompilerContext *ctx, char const *source, char const *end);
  TokenizerResult next(Token *tok);

  void step();
  void skip_whitespace();
  b32 is_at_end() { return at == end; }
};

void Tokenizer::step() {
  if (*at == '\n') {
    current_location.line += 1;
    current_location.col   = 1;
  } else {
    current_location.col += 1;
  }

  current_location.p += 1;
  at                 += 1;
}

b32 is_whitespace(char c) { return (c == ' ') || (c == '\n') || (c == '\r') || (c == '\t'); }
b32 is_numeric(char c) { return c >= '0' && c <= '9'; }
b32 is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
b32 is_identifier_start(char c) { return c == '_' || is_alpha(c); }
b32 is_identifier_rest(char c) { return is_identifier_start(c) || is_numeric(c); }

void Tokenizer::skip_whitespace() {
  while (!is_at_end() && is_whitespace(*at)) {
    step();
  }
}

void Tokenizer::init(CompilerContext *ctx, char const *source, char const *end) {
  this->ctx = ctx;
  this->at  = source;
  this->end = end;

  current_location.line = 1;
  current_location.col  = 1;
  current_location.p    = source;
}

#define Set_out_token(Kind)                                                                        \
  {                                                                                                \
    tok->kind       = Kind;                                                                        \
    tok->span.start = start;                                                                       \
    tok->span.end   = current_location;                                                            \
  }

#define Return_token(Kind)                                                                         \
  {                                                                                                \
    Set_out_token(Kind);                                                                           \
    return TokResult_ok;                                                                           \
  }

#define Return_if_keyword(String, Kind)                                                            \
  if (str_eq(Str_make(String), {start.p, cast<u32>(current_location.p - start.p)})) {              \
    Return_token(Kind);                                                                            \
  }

TokenizerResult Tokenizer::next(Token *tok) {
  skip_whitespace();

  if (is_at_end()) {
    return TokResult_end;
  }

  char c               = *at;
  SourceLocation start = current_location;

  step();

  // clang-format off
  switch (c) {
  case ';': Return_token(Tok_semicolon);
  case ':': Return_token(Tok_colon);
  case '{': Return_token(Tok_brace_open);
  case '}': Return_token(Tok_brace_close);
  case '(': Return_token(Tok_paren_open);
  case ')': Return_token(Tok_paren_close);
  case '+': Return_token(Tok_plus);
  case '*': Return_token(Tok_star);
  case '/': Return_token(Tok_slash);
  case '%': Return_token(Tok_percent);
  case '&': Return_token(Tok_ampersand);
  case '|': Return_token(Tok_bar);
  case '^': Return_token(Tok_caret);
  case '~': Return_token(Tok_tilde);
  }
  // clang-format on

  if (c == '=') {
    if (*at == '=') {
      step();
      Return_token(Tok_cmp_eq);
    }

    Return_token(Tok_equals);
  }

  if (c == '<') {
    if (*at == '=') {
      step();
      Return_token(Tok_cmp_le);
    }

    if (*at == '<') {
      step();
      Return_token(Tok_left_shift);
    }

    Return_token(Tok_cmp_lt);
  }

  if (c == '>') {
    if (*at == '=') {
      step();
      Return_token(Tok_cmp_ge);
    }

    if (*at == '>') {
      step();
      Return_token(Tok_right_shift);
    }

    Return_token(Tok_cmp_gt);
  }

  if (c == '!') {
    if (*at == '=') {
      step();
      Return_token(Tok_cmp_ne);
    }

    Return_token(Tok_exclamation);
  }

  if (c == '-') {
    if (is_at_end()) {
      Return_token(Tok_minus);
    }

    if (*at == '>') {
      step();
      Return_token(Tok_arrow);
    }

    Return_token(Tok_minus);
  }

  if (is_numeric(c)) {
    while (!is_at_end() && is_numeric(*at)) {
      step();
    }

    Return_token(Tok_literal_int);
  }

  if (is_identifier_start(c)) {
    while (!is_at_end() && is_identifier_rest(*at)) {
      step();
    }

    Return_if_keyword("fn", Tok_keyword_fn);
    Return_if_keyword("return", Tok_keyword_return);
    Return_if_keyword("if", Tok_keyword_if);
    Return_if_keyword("else", Tok_keyword_else);
    Return_if_keyword("while", Tok_keyword_while);
    Return_if_keyword("break", Tok_keyword_break);
    Return_if_keyword("continue", Tok_keyword_continue);
    Return_if_keyword("and", Tok_keyword_and);
    Return_if_keyword("or", Tok_keyword_or);

    Return_token(Tok_identifier);
  }

  Str msg = ctx->arena->push_format_string("Unrecognized character '%c'\n", c);
  ctx->messages.push({SourceSpan::from_single_location(start), Error, msg});

  return TokResult_unrecognized_token;
}

b32 tokenize(CompilerContext *ctx, char const *source, usize len, Vector<Token> *tokens) {
  Tokenizer tokenizer;

  tokenizer.init(ctx, source, source + len);

  TokenizerResult res;
  while (true) {
    Token tok;
    res = tokenizer.next(&tok);

    if (res != TokResult_ok) {
      break;
    }

    tokens->push(tok);
  }

  return res == TokResult_end;
}
