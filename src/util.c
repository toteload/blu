#include "toteload.h"

void decode_string_literal(String literal, u8 *out, u32 *len) {
  for (u32 i = 0; i < literal.len; i++) {
    if (literal.str[i] == '\\') {
      Todo(); // actually decode
    }
  }

  memcpy(out+1, literal.str+1, literal.len-2);
  *len = literal.len-2;
}

