#include "toteload.h"
#include "blu.h"
#include "tokens.h"
#include "ast.h"
#include "string_interner.h"
#include "types.h"
#include "value.h"
#include "messages.h"
#include "source_file.h"
#include "compiler.h"
#include "cli_options.h"

#include <stdlib.h>
#include <stdio.h>

internal void write_tokens(Tokens *tokens, String source) {
  for (u32 i = 0; i < tokens->tok_count; i++) {
    u8      kind = tokens->kinds[i];
    SpanU32 span = tokens->spans[i];

    char const *s = Cast(char const*, source.str + span.start);
    int len = Cast(int, span.end - span.start);

    char const *kind_string = token_kind_string(kind);

    printf("%5u:%5u - %s - \"%.*s\"\n", span.start, span.end, kind_string, len, s);
  }
}

internal void write_nodes(AstNodes *nodes, Tokens *tokens, String source) {
  for (u32 i = nodes_begin(nodes); i < nodes_end(nodes); i++) {
    u8 kind = *nodes_kind(nodes, i);
    String kind_string = ast_kind_string(kind);

    printf("%.*s\n", Cast(int, kind_string.len), kind_string.str);
  }
}

internal void *cstd_alloc_fn(void *ctx, void *p, usize old_byte_size, usize new_byte_size, u32 align) {
  if (!is_null(p) && new_byte_size == 0) {
    free(p);
    return Null;
  }

  return realloc(p, new_byte_size);
}

int main(int argc, char const *argv[]) {
  Allocator cstd_allocator = { .fn = cstd_alloc_fn, };

  u32 err = 0;
  b32 ok = False;
  CLIOptions cli = {0};
  err = parse_cli_options(&cli, Cast(u32, argc), argv);
  if (err) {
    printf("Encountered error reading command line options.\n");
    return 1;
  }

  //Arena arena = {0};
  //arena_init(&arena, &(ArenaOptions){
  //  .reserve_size        = MiB(64),
  //  .initial_commit_size = KiB(64),
  //});

  //Arena arena_tmp = {0};
  //arena_init(&arena_tmp, &(ArenaOptions){
  //  .reserve_size        = MiB(64),
  //  .initial_commit_size = KiB(64),
  //});

  //StringInterner strings = {0};
  //strings_init(&strings, &(StringInternerOptions){
  //  .arena         = &arena,
  //  .map_allocator = cstd_allocator,
  //});

  //TypeInterner types = {0};
  //types_init(&types, &(TypeInternerOptions){
  //  .map_allocator = cstd_allocator,
  //  .arena         = &arena,
  //  .arena_scratch = &arena_tmp,
  //});

  //ValueStore values = {0};
  //values_init(&values, &(ValueStoreOptions){
  //  .payload_allocator = cstd_allocator,
  //});

  Compiler compiler;
  compiler_init(&compiler);

  compiler_add_sourcefile(&compiler, cli.source_filename);

  ok = compile(&compiler);
  compiler_print_all_messages(&compiler);

  printf("%s\n", (ok) ? "ok" : "error");

  return 0;
}
