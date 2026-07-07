#include "blu.h"
#include "tokens.h"
#include "source_file.h"

#define SEGMENTLIST_NAME            KindList
#define SEGMENTLIST_TYPE            u8
#define SEGMENTLIST_FUNCTION_PREFIX kinds
#define SEGMENTLIST_MIN_SIZE_LOG2   KindList_min_size_log2
#define SEGMENTLIST_SEGMENT_COUNT   KindList_segment_count
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

#define SEGMENTLIST_NAME            SpanList
#define SEGMENTLIST_TYPE            SpanU32
#define SEGMENTLIST_FUNCTION_PREFIX spans
#define SEGMENTLIST_MIN_SIZE_LOG2   SpanList_min_size_log2
#define SEGMENTLIST_SEGMENT_COUNT   SpanList_segment_count
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

#define SEGMENTLIST_NAME            OffsetList
#define SEGMENTLIST_TYPE            u32
#define SEGMENTLIST_FUNCTION_PREFIX lines
#define SEGMENTLIST_MIN_SIZE_LOG2   OffsetList_min_size_log2
#define SEGMENTLIST_SEGMENT_COUNT   OffsetList_segment_count
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

enum TokenizerResult {
  TokResult_ok,
  TokResult_end,
  TokResult_error,
};

typedef struct {
  u8 const *start;
  u8 const *end;
  u8 const *at;

  Source *source;
} Tokenizer;

internal b32 is_whitespace_except_newline(u8 c) { return (c == ' ') || (c == '\r') || (c == '\t'); }
internal b32 is_numeric(u8 c) { return c >= '0' && c <= '9'; }
internal b32 is_alpha(u8 c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
internal b32 is_identifier_start(u8 c) { return c == '_' || is_alpha(c); }
internal b32 is_identifier_rest(u8 c) { return is_identifier_start(c) || is_numeric(c); }

internal b32 is_at_end(Tokenizer *tokenizer) {
  return tokenizer->at == tokenizer->end;
}

internal void skip_whitespace(Tokenizer *tokenizer) {
  while (!is_at_end(tokenizer) && is_whitespace_except_newline(*tokenizer->at)) {
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
    span->start = Cast(u32, token_start - tokenizer->start);                                       \
    span->end   = Cast(u32, tokenizer->at - tokenizer->start);                                     \
    return TokResult_ok;                                                                           \
  }

#define Return_if_match(s, Kind)                                                                   \
  if (string_eq(string_lit(s), (String){.str = token_start, .len = Cast(usize, tokenizer->at - token_start)})) { \
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
  case '\n': Return_token(Tok_newline);
  }
  // clang-format on

  if (c == ';') {
    step_until_new_line(tokenizer);
    tokenizer->at++;
    Return_token(Tok_line_comment);
  }

  if (c == '-') {
    if (is_at_end(tokenizer)) {
      Return_token(Tok_minus);
    }

    if (*tokenizer->at == '>') {
      tokenizer->at += 1;
      Return_token(Tok_arrow);
    }

    if (is_numeric(*tokenizer->at)) {
      tokenizer->at += 1;
      while (!is_at_end(tokenizer) && is_numeric(*tokenizer->at)) {
        tokenizer->at += 1;
      }

      Return_token(Tok_literal_int);
    }

    Return_token(Tok_minus);
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
      error(
        tokenizer->source,
        (MessageLocation){ .kind = MessageLocation_end_of_file },
        string_lit("End of source encountered while parsing string literal.")
      );
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
    Return_if_match("bitcast",  Tok_keyword_bitcast);
    Return_if_match("as",       Tok_keyword_as);
    Return_if_match("mod",      Tok_keyword_mod);
    Return_if_match("no_cache", Tok_keyword_no_cache);
    Return_if_match("inline",   Tok_keyword_inline);
    Return_if_match("#print",   Tok_builtin_print);
    // clang-format on

    Return_token(Tok_identifier);
  }

  u32 offset = Cast(u32, ptr_diff(token_start, tokenizer->start));

  error(
    tokenizer->source, 
    (MessageLocation){
      .kind = MessageLocation_byte_offset,
      .data.offset = offset,
    },
    string_lit("Unrecognized token encountered.")
  );

  return TokResult_error;
}

void tokens_init(Tokens *tokens, Arena *arena) {
  zero_struct(Tokens, tokens);

  kinds_push(&tokens->kinds, arena);
  spans_push(&tokens->spans, arena);
  lines_append(&tokens->lines, arena, 0);
}

u32 tokens_count(Tokens *tokens) {
  return tokens->kinds.len - 1;
}

u32 tokens_begin(Tokens *tokens) {
  return 1;
}

u32 tokens_end(Tokens *tokens) {
  return tokens->kinds.len;
}

LineInfo tokens_find_line_info(Tokens *tokens, u32 byte_offset) {
  // OPTIMIZE: A binary search is probably faster for bigger files.
  u32 len = tokens->lines.len;
  for (u32 i = 1; i < len; i++) {
    u32 offset = *lines_ptr_at_unchecked(&tokens->lines, i);
    if (offset > byte_offset) {
      u32 offset_prev = *lines_ptr_at_unchecked(&tokens->lines, i-1);
      return (LineInfo){
        .line = i,
        .offset_start_of_line = offset_prev,
        .line_len = offset - offset_prev,
      };
    }
  }

  return (LineInfo){0};
}

TokenIndex tokens_alloc(Tokens *tokens, Arena *arena) {
  TokenIndex idx = tokens->kinds.len;
  kinds_push(&tokens->kinds, arena);
  spans_push(&tokens->spans, arena);
  return idx;
}

u8 *tokens_kind(Tokens *tokens, TokenIndex idx) {
  return kinds_ptr_at_unchecked(&tokens->kinds, idx);
}

SpanU32 *tokens_span(Tokens *tokens, TokenIndex idx) {
  return spans_ptr_at_unchecked(&tokens->spans, idx);
}

b32 source_tokenize(Source *source) {
  Tokenizer tokenizer = {
    .start  = source->text.str,
    .end    = ptr_offset(source->text.str, source->text.len),
    .at     = source->text.str,
    .source = source,
  };

  Tokens *tokens = &source->tokens;

  u32 res;
  while (True) {
    u8 kind;
    SpanU32 span;
    res = next(&tokenizer, &kind, &span);

    if (res != TokResult_ok) {
      break;
    }

    if (kind == Tok_line_comment || kind == Tok_newline) {
      lines_append(&tokens->lines, &source->arena, span.end);
      continue;
    }

    TokenIndex i = tokens_alloc(tokens, &source->arena);

    *tokens_kind(tokens, i) = kind;
    *tokens_span(tokens, i) = span;
  }

  lines_append(&tokens->lines, &source->arena, source->text.len);

  return res == TokResult_end;
}

char const *token_kind_string_literals[Tok_kind_max] = {
  "colon",        "semicolon",
  "comma",        "dot", "arrow",
  "equals",       "minus",
  "plus",         "star",
  "slash",        "percent",
  "plus_equals",  "exclamation",
  "ampersand",    "bar",
  "caret",        "tilde",
  "left-shift",   "right-shift",
  "cmp-eq",       "cmp-ne",
  "cmp-gt",       "cmp-ge",
  "cmp-lt",       "cmp-le",
  "literal-int",  "literal-string",
  "brace-open",   "brace-close",
  "paren-open",   "paren-close",
  "bracket-open", "bracket-close",
  "if",           "else",
  "for",          "do",
  "break",        "continue",
  "return",       "and",
  "or",           "defer",
  "const",        "cast",
  "bitcast", "as", "mod", "no_cache", "inline",
  "identifier",   "#print",
  "line-comment",
};

char const *token_kind_string(u8 kind) {
  if (kind >= Tok_kind_max) {
    return "<illegal-token-kind>";
  }

  return token_kind_string_literals[kind];
}
