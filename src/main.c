#include "toteload.h"
#include "blu.h"
#include "tokens.h"

int main(int argc, char const *argv[]) {
  Arena arena = {};
  arena_init(&arena);

  Arena arena_tmp = {};
  arena_init(&arena_tmp);

  Tokens tokens = {};
  tokens_init(&tokens);

  AstNodes nodes = {};
  nodes_init(&nodes);

  StringInterner strings = {};
  strings_init(&strings);

  TypeInterner types = {};
  types_init(&types);

  ValueStore values = {};
  values_init(&values);

  return 0;
}
