#include "blu.h"
#include "tokens.h"
#include "source_file.h"

#define SEGMENTLIST_NAME            KindList
#define SEGMENTLIST_TYPE            u8
#define SEGMENTLIST_FUNCTION_PREFIX kindlist
#define SEGMENTLIST_LINKAGE         internal
#define SEGMENTLIST_MIN_SIZE_LOG2   8
#define SEGMENTLIST_SEGMENT_COUNT   24
#define SEGMENTLIST_OUTPUT_TYPES
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

#define SEGMENTLIST_NAME            SpanList
#define SEGMENTLIST_TYPE            SpanU32
#define SEGMENTLIST_FUNCTION_PREFIX spanlist
#define SEGMENTLIST_LINKAGE         internal
#define SEGMENTLIST_MIN_SIZE_LOG2   8
#define SEGMENTLIST_SEGMENT_COUNT   24
#define SEGMENTLIST_OUTPUT_TYPES
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

#define SEGMENTLIST_NAME            OffsetList
#define SEGMENTLIST_TYPE            u32
#define SEGMENTLIST_FUNCTION_PREFIX lines
#define SEGMENTLIST_LINKAGE         internal
#define SEGMENTLIST_MIN_SIZE_LOG2   8
#define SEGMENTLIST_SEGMENT_COUNT   24
#define SEGMENTLIST_OUTPUT_TYPES
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

  MessageSink *msg_sink;
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
  case '{': Return_token(Tok_brace_open);
  case '}': Return_token(Tok_brace_close);
  case '(': Return_token(Tok_paren_open);
  case ')': Return_token(Tok_paren_close);
  case '[': Return_token(Tok_bracket_open);
  case ']': Return_token(Tok_bracket_close);
  case '/': Return_token(Tok_slash);
  case '&': Return_token(Tok_ampersand);
  case '|': Return_token(Tok_bar);
  case '^': Return_token(Tok_caret);
  case '~': Return_token(Tok_tilde);
  case '\n': Return_token(Tok_newline);
  }
  // clang-format on

  if (c == ';') {
    step_until_new_line(tokenizer);
    Return_token(Tok_line_comment);
  }

  if (c == ':') {
    if (is_at_end(tokenizer)) {
      Return_token(Tok_colon);
    }

    if (*tokenizer->at != ':') {
      Return_token(Tok_colon);
    }

    tokenizer->at += 1;
    if (is_at_end(tokenizer)) {
      Message_error(
        tokenizer->msg_sink,
        (MessageLocation){ 
          .kind = MessageLocation_byte_offset,
          .data.offset = Cast(u32, tokenizer->end - tokenizer->start),
        },
        string_lit("End of source encountered while parsing label")
      );
      return TokResult_error;
    }

    if (!is_identifier_start(*tokenizer->at)) {
      Message_error(
        tokenizer->msg_sink,
        (MessageLocation){ 
          .kind = MessageLocation_byte_offset,
          .data.offset = Cast(u32, tokenizer->end - tokenizer->start),
        },
        string_lit("Expected name while parsing label")
      );
      return TokResult_error;
    }

    tokenizer->at += 1;

    while (!is_at_end(tokenizer) && is_identifier_rest(*tokenizer->at)) {
      tokenizer->at += 1;
    }

    Return_token(Tok_label);
  }

  if (c == '-') {
    if (is_at_end(tokenizer)) {
      Return_token(Tok_minus);
    }

    if (*tokenizer->at == '=') {
      tokenizer->at += 1;
      Return_token(Tok_minus_equals);
    }

    Return_token(Tok_minus);
  }

  if (c == '=') {
    if (is_at_end(tokenizer)) {
      Return_token(Tok_equals);
    }

    if (*tokenizer->at == '=') {
      tokenizer->at += 1;
      Return_token(Tok_cmp_eq);
    }

    Return_token(Tok_equals);
  }

  if (c == '+') {
    if (is_at_end(tokenizer)) {
      Return_token(Tok_plus);
    }

    if (*tokenizer->at == '=') {
      tokenizer->at += 1;
      Return_token(Tok_plus_equals);
    }

    Return_token(Tok_plus);
  }

  if (c == '*') {
    if (is_at_end(tokenizer)) {
      Return_token(Tok_star);
    }

    if (*tokenizer->at == '=') {
      tokenizer->at += 1;
      Return_token(Tok_star_equals);
    }

    Return_token(Tok_star);
  }

  if (c == '%') {
    if (is_at_end(tokenizer)) {
      Return_token(Tok_percent);
    }

    if (*tokenizer->at == '=') {
      tokenizer->at += 1;
      Return_token(Tok_percent_equals);
    }

    Return_token(Tok_percent);
  }

  if (c == '<') {
    if (is_at_end(tokenizer)) {
      Return_token(Tok_cmp_lt);
    }

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
    if (is_at_end(tokenizer)) {
      Return_token(Tok_cmp_gt);
    }

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
    if (is_at_end(tokenizer)) {
      Return_token(Tok_exclamation);
    }

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
      Message_error(
        tokenizer->msg_sink,
        (MessageLocation){ 
          .kind = MessageLocation_byte_offset,
          .data.offset = Cast(u32, tokenizer->end - tokenizer->start),
        },
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

    if (c == '#') {
      Return_if_match("#debug", Tok_builtin_debug);

      Message_error(
        tokenizer->msg_sink,
        (MessageLocation){
          .kind = MessageLocation_byte_offset,
          .data.offset = Cast(u32, ptr_diff(token_start, tokenizer->start)),
        },
        string_lit("Unrecognized builtin encountered.")
      );

      return TokResult_error;
    }

    // clang-format off
    Return_if_match("return",   Tok_keyword_return);
    Return_if_match("if",       Tok_keyword_if);
    Return_if_match("else",     Tok_keyword_else);
    Return_if_match("for",      Tok_keyword_for);
    Return_if_match("while",    Tok_keyword_while);
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
    // clang-format on

    Return_token(Tok_identifier);
  }

  Message_error(
    tokenizer->msg_sink,
    (MessageLocation){
      .kind = MessageLocation_byte_offset,
      .data.offset = Cast(u32, ptr_diff(token_start, tokenizer->start)),
    },
    string_lit("Unrecognized token encountered.")
  );

  return TokResult_error;
}

LineInfo tokens_find_line_info(String text, Tokens *tokens, u32 byte_offset) {
  // OPTIMIZE: A binary search is probably faster for big enough files and a better default.
  u32 len = tokens->line_count;
  for (u32 i = 1; i < len; i++) {
    u32 offset = tokens->lines[i];
    if (offset > byte_offset) {
      u32 offset_prev = tokens->lines[i-1];
      return (LineInfo){
        .line = i,
        .offset_start_of_line = offset_prev,
        .line_len = offset - offset_prev,
      };
    }
  }

  u32 offset = text.len;
  for (u32 i = byte_offset; i < text.len; i++) {
    if (text.str[i] == '\n') {
      offset = i;
      break;
    }
  }

  return (LineInfo){
    .line = len,
    .offset_start_of_line = tokens->lines[len-1],
    .line_len = offset - tokens->lines[len-1],
  };
}

u8 tokens_kind(Tokens *tokens, TokenIndex idx) {
  return tokens->kinds[idx];
}

SpanU32 tokens_span(Tokens *tokens, TokenIndex idx) {
  return tokens->spans[idx];
}

b32 tokenize(TokenizeContext *context, String text, Tokens *tokens) {
  Tokenizer tokenizer = {
    .start    = text.str,
    .end      = ptr_offset(text.str, text.len),
    .at       = text.str,
    .msg_sink = context->msg_sink,
  };

  ArenaSnapshot scope = arena_scope_begin(context->scratch);

  KindList kindlist = {0};
  SpanList spanlist = {0};
  OffsetList linelist = {0};

  lines_append(&linelist, context->scratch, 0);

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

    if (kind == Tok_newline) {
      lines_append(&linelist, context->scratch, span.end);
      continue;
    }

    kindlist_append(&kindlist, context->scratch, kind);
    spanlist_append(&spanlist, context->scratch, span);
  }

  u32 tok_count  = kindlist.len;
  u32 line_count = linelist.len;

  u8      *kinds = arena_push_array(u8,      context->arena, tok_count);
  SpanU32 *spans = arena_push_array(SpanU32, context->arena, tok_count);

  kindlist_copy_to_array(&kindlist, kinds);
  spanlist_copy_to_array(&spanlist, spans);

  u32 *lines = arena_push_array(u32, context->arena, line_count);
  lines_copy_to_array(&linelist, lines);

  *tokens = (Tokens){
    .tok_count  = tok_count,
    .line_count = line_count,
    .kinds = kinds,
    .spans = spans,
    .lines = lines,
  };

  arena_scope_end(context->scratch, scope);

  return res == TokResult_end;
}

String token_string(Tokens *tokens, String text, TokenIndex tok) {
  SpanU32 span = tokens->spans[tok];
  return (String){ .str = text.str + span.start, .len = span.end - span.start };
}

char const *token_kind_string_literals[Tok_kind_max] = {
  ":",        ";",
  ",",        ".",
  "=",       "-",
  "+",         "*",
  "/",        "%",
  "-=",
  "+=", "*=", "%=",  "!",
  "&",    "|",
  "^",        "~",
  "<<",   ">>",
  "==",       "!=",
  ">",       ">=",
  "<",       "<=",
  "integer literal",  "string literal",
  "{",   "}",
  "(",   ")",
  "[", "]",
  "if",           "else",
  "for",          "while", "do",
  "break",        "continue",
  "return",       "and",
  "or",           "defer",
  "const",        "cast",
  "bitcast", "as", "mod", "no_cache", "inline",
  "identifier",   "label", "#debug",
  "line comment", "newline",
};

char const *token_kind_string(u8 kind) {
  if (kind >= Tok_kind_max) {
    return "<illegal-token-kind>";
  }

  return token_kind_string_literals[kind];
}
