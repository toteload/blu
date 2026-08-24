#include "toteload.h"

u32 decode_string_literal(String literal, u8 *out, u32 *len) {
  // TODO: actually decode
  for (u32 i = 0; i < literal.len; i++) {
    if (literal.str[i] == '\\') {
      Todo();
    }
  }

  memcpy(out+1, literal.str+1, literal.len-2);
  *len = literal.len-2;

  return 0;
}

