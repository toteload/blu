#include "toteload.h"
#include "blu.h"
#include "tokens.h"
#include "ast.h"
#include "string_interner.h"
#include "types.h"
#include "value.h"

#include <stdlib.h>

internal void *cstd_alloc_fn(void *ctx, void *p, usize old_byte_size, usize new_byte_size, u32 align) {
  if (!is_null(p) && new_byte_size == 0) {
    free(p);
    return Null;
  }

  return realloc(p, new_byte_size);
}

int main(int argc, char const *argv[]) {
  Allocator cstd_allocator = { .fn = cstd_alloc_fn, };

  Arena arena = {0};
  arena_init(&arena, &(ArenaOptions){
    .reserve_size        = MiB(64),
    .initial_commit_size = KiB(64),
  });

  Arena arena_tmp = {0};
  arena_init(&arena_tmp, &(ArenaOptions){
    .reserve_size        = MiB(64),
    .initial_commit_size = KiB(64),
  });

  Tokens tokens = {0};
  tokens_init(&tokens);

  AstNodes nodes = {0};
  nodes_init(&nodes);

  StringInterner strings = {0};
  strings_init(&strings, &(StringInternerOptions){
    .arena         = &arena,
    .map_allocator = cstd_allocator,
  });

  TypeInterner types = {0};
  types_init(&types, &(TypeInternerOptions){
    .arena         = &arena,
    .map_allocator = cstd_allocator,
  });

  ValueStore values = {0};
  values_init(&values, &(ValueStoreOptions){
    .arena             = &arena,
    .payload_allocator = cstd_allocator,
  });

  return 0;
}
