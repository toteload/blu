#include "blu.hh"
#include <stdio.h>

void debug_print_type(TypeInterner *types, TypeIndex type) {
  char buf[256] = {0};
  u32 len = type_to_string(types, type, buf, 256);
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
