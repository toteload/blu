#include "blu.hh"

struct Tokenizer {
  char const *at;
  char const *end;

  SourceLocation current_location;

  void init(char const *source, char const *end);
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
  at += 1;
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

void Tokenizer::init(char const *source, char const *end) {
  this->at  = source;
  this->end = end;

  current_location.line   = 1;
  current_location.col    = 1;
  current_location.p = source;
}

#define Set_out_token(Kind)                                                                        \
  {                                                                                                \
    tok->kind       = Kind;                                                                        \
    tok->span.start = start;                                                                       \
    tok->span.end   = current_location;                                                            \
  }

#define Return_single_character_token(Kind)                                                        \
  {                                                                                                \
    step();                                                                                        \
    Set_out_token(Kind);                                                                           \
    return TokResult_ok;                                                                           \
  }

#define Return_if_keyword(String, Kind) \
  if (memcmp(String, start.p, current_location.p - start.p) == 0) { Set_out_token(Kind); return TokResult_ok; } 

TokenizerResult Tokenizer::next(Token *tok) {
  skip_whitespace();

  if (is_at_end()) {
    return TokResult_end;
  }

  char c = *at;

  SourceLocation start = current_location;

  // clang-format off
  switch (c) {
  case ':': Return_single_character_token(Tok_colon);
  case ';': Return_single_character_token(Tok_semicolon);
  case '=': Return_single_character_token(Tok_equals);
  case '{': Return_single_character_token(Tok_brace_open);
  case '}': Return_single_character_token(Tok_brace_close);
  case '(': Return_single_character_token(Tok_paren_open);
  case ')': Return_single_character_token(Tok_paren_close);
  }
  // clang-format on

  if (c == '-') {
    if (at + 1 == end || (!is_numeric(*(at+1)))) {
      Return_single_character_token(Tok_minus);
    }

    step();
    c = *at;

    // `c` is now set to the character after the minus, and the next character is numeric. We will
    // fall into the literal int parsing branch below and include the minus as part of the literal.
  }

  if (is_numeric(c)) {
    step();

    while (!is_at_end() && is_numeric(*at)) {
      step();
    }

    Set_out_token(Tok_literal_int);

    return TokResult_ok;
  }

  if (is_identifier_start(c)) {
    step();

    while (!is_at_end() && is_identifier_rest(*at)) {
      step();
    }

    Return_if_keyword("fn", Tok_keyword_fn);
    Return_if_keyword("return", Tok_keyword_return);

    Set_out_token(Tok_identifier);

    return TokResult_ok;
  }

  return TokResult_unrecognized_token;
}

b32 tokenize(char const *source, usize len, Vector<Token> *tokens) {
  Tokenizer tokenizer;

  tokenizer.init(source, source + len);

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

