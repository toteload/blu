#include "toteload.h"
#include "blu.h"
#include "tokens.h"
#include "ast.h"
#include "string_interner.h"
#include "types.h"
#include "value.h"
#include "messages.h"
#include "check.h"
#include "env.h"

#include <stdlib.h>
#include <stdio.h>

typedef struct {
  b32 verbose;
  String source_filename;
} CLIOptions;

enum ParseCliResult {
  ParseCli_ok,
  ParseCli_error_unknown_argument,
  ParseCli_error_no_source_filename_provided,
};

u32 parse_cli_options(CLIOptions *options, u32 arg_count, char const *const *args) {
  *options = (CLIOptions){
    .verbose = False,
    .source_filename = {0},
  };

  b32 has_found_source_filename = False;

  for (u32 i = 1; i < arg_count; i++) {
    String arg = string_from_cstr(args[i]);

    if (string_eq(arg, string_lit("-v"))) {
      options->verbose = True;
      continue;
    } 

    if (!has_found_source_filename) {
      has_found_source_filename = True;
      options->source_filename = arg;
      continue;
    }

    return ParseCli_error_unknown_argument;
  }

  if (!has_found_source_filename) {
    return ParseCli_error_no_source_filename_provided;
  }

  return ParseCli_ok;
}

internal void write_tokens(Tokens *tokens, String source) {
  for (u32 i = tokens_begin(tokens); i < tokens_end(tokens); i++) {
    u8      kind = *tokens_kind(tokens, i);
    SpanU32 span = *tokens_span(tokens, i);

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

  StringInterner strings = {0};
  strings_init(&strings, &(StringInternerOptions){
    .arena         = &arena,
    .map_allocator = cstd_allocator,
  });

  TypeInterner types = {0};
  types_init(&types, &(TypeInternerOptions){
    .map_allocator = cstd_allocator,
    .arena         = &arena,
    .arena_scratch = &arena_tmp,
  });

  // root
  // |
  // |- mod main
  // |  \- fn main
  // |
  // \- mod util
  //    \- fn print

  ValueStore values = {0};
  values_init(&values, &(ValueStoreOptions){
    .payload_allocator = cstd_allocator,
  });

  SourceAllocator sources;
  sources_init(&sources);

  Source *source;
  SourceIndex source_index = sources_alloc_and_get(&sources, &source);
  Unused(source_index);
  source_file_init(source, cli.source_filename);

  ok = source_read_file(source);
  if (!ok) { source_print_all_messages(source); return 1; }

  ok = source_tokenize(source);
  if (cli.verbose) {
    write_tokens(&source->tokens, source->text);
  }
  if (!ok) { source_print_all_messages(source); return 1; }

  ok = source_parse(source);
  if (cli.verbose) {
    write_nodes(&source->ast, &source->tokens, source->text);
  }
  if (!ok) { source_print_all_messages(source); return 1; }

  EnvAllocator envs;
  envs_init(&envs, &(EnvAllocatorOptions){
    .arena         = &arena,
    .map_allocator = cstd_allocator,
  });

  Messages messages;
  messages_init(&messages);

  Checker checker;
  checker_init(&checker, &(CheckerOptions){
    .messages = &messages,
    .strings  = &strings,
    .types    = &types,
    .envs     = &envs,

    .source_count = 1,
    .sources      = source,
  });

  ok = check_code(&checker);
  if (!ok) {
    messages_print_all_messages(&messages, &sources);
    return 1;
  }

  printf("ok\n");

  return 0;
}
