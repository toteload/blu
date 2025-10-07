#include "blu.hh"
#include <stdio.h>

void debug_print_type(TypeInterner *types, TypeIndex type) {
  char buf[256] = {0};
  u32 len = type_to_string(types, type, Slice<char>::from_ptr_and_len(buf, 256));
  printf("%.*s\n", len, buf);
}

