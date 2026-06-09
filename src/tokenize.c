#include "blu.h"
#include "tokens.h"
#include "messages.h"

enum TokenizerResult {
  TokResult_ok,
  TokResult_end,
  TokResult_error,
};

typedef struct {
  u8 const *source;
  u8 const *end;
  u8 const *at;

  Messages *messages;
} Tokenizer;

internal b32 is_whitespace(u8 c) { return (c == ' ') || (c == '\n') || (c == '\r') || (c == '\t'); }
internal b32 is_numeric(u8 c) { return c >= '0' && c <= '9'; }
internal b32 is_alpha(u8 c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
internal b32 is_identifier_start(u8 c) { return c == '_' || is_alpha(c); }
internal b32 is_identifier_rest(u8 c) { return is_identifier_start(c) || is_numeric(c); }

internal b32 is_at_end(Tokenizer *tokenizer) {
  return tokenizer->at == tokenizer->end;
}

internal void skip_whitespace(Tokenizer *tokenizer) {
  while (!is_at_end(tokenizer) && is_whitespace(*tokenizer->at)) {
    tokenizer->at += 1;
  }
}

internal void step_until_new_line(Tokenizer *tokenizer) {
  while (!is_at_end(tokenizer) && *tokenizer->at != '\n') {
    tokenizer->at += 1;
  }
}

#define Return_token(Kind)                                                                         \
  {                                                                                                \
    *kind       = Kind;                                                                            \
    span->start = Cast(u32, token_start - tokenizer->source);                                                  \
    span->end   = Cast(u32, tokenizer->at - tokenizer->source);                                                           \
    return TokResult_ok;                                                                           \
  }

#define Return_if_match(s, Kind)                                                              \
  if (string_eq(string_lit(s), (String){.str = token_start, .len = Cast(usize, tokenizer->at - token_start)})) {                    \
    Return_token(Kind);                                                                            \
  }

internal u32 next(Tokenizer *tokenizer, u8 *kind, SpanU32 *span) {
  skip_whitespace(tokenizer);

  if (is_at_end(tokenizer)) {
    return TokResult_end;
  }

  u8        c           = *tokenizer->at;
  u8 const *token_start = tokenizer->at;

  tokenizer->at++;

  // clang-format off
  switch (c) {
  case ',': Return_token(Tok_comma);
  case '.': Return_token(Tok_dot);
  case ':': Return_token(Tok_colon);
  case '{': Return_token(Tok_brace_open);
  case '}': Return_token(Tok_brace_close);
  case '(': Return_token(Tok_paren_open);
  case ')': Return_token(Tok_paren_close);
  case '[': Return_token(Tok_bracket_open);
  case ']': Return_token(Tok_bracket_close);
  case '*': Return_token(Tok_star);
  case '/': Return_token(Tok_slash);
  case '%': Return_token(Tok_percent);
  case '&': Return_token(Tok_ampersand);
  case '|': Return_token(Tok_bar);
  case '^': Return_token(Tok_caret);
  case '~': Return_token(Tok_tilde);
  case '-': Return_token(Tok_minus);
  }
  // clang-format on

  if (c == ';') {
    step_until_new_line(tokenizer);
    Return_token(Tok_line_comment);
  }

  if (c == '=') {
    if (*tokenizer->at == '=') {
      tokenizer->at += 1;
      Return_token(Tok_cmp_eq);
    }

    Return_token(Tok_equals);
  }

  if (c == '+') {
    if (*tokenizer->at == '=') {
      tokenizer->at += 1;
      Return_token(Tok_plus_equals);
    }

    Return_token(Tok_plus);
  }

  if (c == '<') {
    if (*tokenizer->at == '=') {
      tokenizer->at += 1;
      Return_token(Tok_cmp_le);
    }

    if (*tokenizer->at == '<') {
      tokenizer->at += 1;
      Return_token(Tok_left_shift);
    }

    Return_token(Tok_cmp_lt);
  }

  if (c == '>') {
    if (*tokenizer->at == '=') {
      tokenizer->at += 1;
      Return_token(Tok_cmp_ge);
    }

    if (*tokenizer->at == '>') {
      tokenizer->at += 1;
      Return_token(Tok_right_shift);
    }

    Return_token(Tok_cmp_gt);
  }

  if (c == '!') {
    if (*tokenizer->at == '=') {
      tokenizer->at += 1;
      Return_token(Tok_cmp_ne);
    }

    Return_token(Tok_exclamation);
  }

  if (c == '"') {
    while (!is_at_end(tokenizer) && *tokenizer->at != '"') {
      if (*tokenizer->at == '\\') {
        tokenizer->at += 1;
        if (is_at_end(tokenizer)) {
          break;
        }
      }
      tokenizer->at += 1;
    }

    if (is_at_end(tokenizer)) {
      messages_add_error(tokenizer->messages, string_lit("End of source encountered while parsing string literal."));
      return TokResult_error;
    }

    tokenizer->at += 1;

    Return_token(Tok_literal_string);
  }

  if (is_numeric(c)) {
    while (!is_at_end(tokenizer) && is_numeric(*tokenizer->at)) {
      tokenizer->at += 1;
    }

    Return_token(Tok_literal_int);
  }

  if (is_identifier_start(c) || c == '#') {
    while (!is_at_end(tokenizer) && is_identifier_rest(*tokenizer->at)) {
      tokenizer->at += 1;
    }

    // clang-format off
    Return_if_match("return",   Tok_keyword_return);
    Return_if_match("if",       Tok_keyword_if);
    Return_if_match("else",     Tok_keyword_else);
    Return_if_match("for",      Tok_keyword_for);
    Return_if_match("do",       Tok_keyword_do);
    Return_if_match("break",    Tok_keyword_break);
    Return_if_match("continue", Tok_keyword_continue);
    Return_if_match("and",      Tok_keyword_and);
    Return_if_match("or",       Tok_keyword_or);
    Return_if_match("defer",    Tok_keyword_defer);
    Return_if_match("const",    Tok_keyword_const);
    Return_if_match("cast",     Tok_keyword_cast);

    Return_if_match("#print",   Tok_builtin_print);
    // clang-format on

    Return_token(Tok_identifier);
  }

  messages_add_error(tokenizer->messages, string_lit("Unrecognized token encountered."));

  return TokResult_error;
}

void tokens_init(Tokens *tokens) {
  arena_init(&tokens->kinds, &(ArenaOptions){
    .reserve_size = MiB(1),
    .initial_commit_size = KiB(16),
  });
  arena_init(&tokens->spans, &(ArenaOptions){
    .reserve_size = MiB(1),
    .initial_commit_size = KiB(8 * 16),
  });

  arena_push_array(&tokens->kinds, u8, 1);
  arena_push_array(&tokens->spans, SpanU32, 1);

  tokens->offset = 1;
}

void tokens_deinit(Tokens *tokens) {
  arena_deinit(&tokens->kinds);
  arena_deinit(&tokens->spans);

  zero_struct(Tokens, tokens);
}

u32 tokens_end(Tokens *tokens) {
  return tokens->offset;
}

u32 tokens_count(Tokens *tokens) {
  return tokens->offset - 1;
}

TokenIndex tokens_alloc(Tokens *tokens) {
  TokenIndex idx = tokens->offset;
  tokens->offset += 1;
  arena_push_array(&tokens->kinds, u8, 1);
  arena_push_array(&tokens->spans, SpanU32, 1);
  return idx;
}

u8 *tokens_kind(Tokens *tokens, TokenIndex idx) {
  return Cast(u8*, tokens->kinds.base) + idx;
}

SpanU32 *tokens_span(Tokens *tokens, TokenIndex idx) {
  return Cast(SpanU32*, tokens->spans.base) + idx;
}

b32 tokenize(Messages *messages, Tokens *tokens, String source) {
  Tokenizer tokenizer = {
    .source   = source.str,
    .end      = ptr_offset(source.str, source.len),
    .at       = source.str,
    .messages = messages,
  };

  u32 res;
  while (True) {
    u8 kind;
    SpanU32 span;
    res = next(&tokenizer, &kind, &span);

    if (res != TokResult_ok) {
      break;
    }

    if (kind == Tok_line_comment) {
      continue;
    }

    TokenIndex i = tokens_alloc(tokens);

    *tokens_kind(tokens, i) = kind;
    *tokens_span(tokens, i) = span;
  }

  return res == TokResult_end;
}

