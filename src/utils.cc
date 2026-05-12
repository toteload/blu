#include "blu.hh"
#include <stdio.h>

void debug_print_type(TypeInterner *types, TypeIndex type) {
  char buf[256] = {0};
  u32 len = types->type_to_string(type, buf, 256);
  printf("%.*s\n", len, buf);
}

void write_tokens(Tokens *tokens, Str source, Arena *out) {
  for (u32 i = 0; i < tokens->len(); i += 1) {
    TokenIndex idx = { i };
    auto span = tokens->span(idx);
    Str s = source.sub(span.start, span.end);
    out->push_format_string("%s - span(%d:%d) - \"%.*s\"\n", token_kind_string(tokens->kind(idx)), span.start, span.end, (int)s.len(), s.str);
  }
}

u32 string_literal_byte_size(Str literal) {
  u32 size = 0;
  for (usize i = 1; i < literal.len() - 1; i += 1) {
    if (literal[i] != '\\') {
      size += 1;
      continue;
    }

    i += 1;
    switch (literal[i]) {
    case 'n': case 't': case 'r': case '0':
    case '\\': case '\'': case '"':
    case 'a': case 'b': case 'f': case 'v':
      size += 1;
      break;
    default:
      size += 2;
      break;
    }
  }
  return size;
}

// Assume: the string literal is wellformed.
// Unrecognized escape codes are written as is.
u32 decode_string_literal(Str literal, char *out) {
  u32 written = 0;
  for (usize i = 1; i < literal.len() - 1; i += 1) {
    char c = literal[i];
    if (c != '\\') {
      out[written++] = c;
      continue;
    }

    i += 1;
    char escaped = literal[i];
    switch (escaped) {
    case 'n':  out[written++] = '\n'; break;
    case 't':  out[written++] = '\t'; break;
    case 'r':  out[written++] = '\r'; break;
    case '0':  out[written++] = '\0'; break;
    case '\\': out[written++] = '\\'; break;
    case '\'': out[written++] = '\''; break;
    case '"':  out[written++] = '"';  break;
    case 'a':  out[written++] = '\a'; break;
    case 'b':  out[written++] = '\b'; break;
    case 'f':  out[written++] = '\f'; break;
    case 'v':  out[written++] = '\v'; break;
    default:
      out[written++] = '\\';
      out[written++] = escaped;
      break;
    }
  }

  return written;
}

