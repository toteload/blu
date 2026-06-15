#include "toteload.h"
#include "blu.h"
#include "tokens.h"
#include "ast.h"
#include "string_interner.h"
#include "types.h"
#include "value.h"
#include "messages.h"

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

enum ReadFileResult {
  ReadFile_ok,
  ReadFile_error_could_not_open_file,
  ReadFile_error_unexpected_content_size,
  ReadFile_error_ftell_error,
};

u32 read_file(String filename, Arena *arena, String *content) {
  FILE *f;
  {
    ArenaSnapshot scope = arena_scope_begin(arena);
    u8 *buf = arena_push_array(u8, arena, filename.len + 1);
    memcpy(buf, filename.str, filename.len);
    buf[filename.len] = '\0';

    f = fopen(Cast(char const *, buf), "rb");
    if (is_null(f)) {
      arena_scope_end(arena, scope);
      return ReadFile_error_could_not_open_file;
    }

    arena_scope_end(arena, scope);
  }

  fseek(f, 0, SEEK_END);

  ArenaSnapshot savepoint = arena_scope_begin(arena);

  i32 sizei = ftell(f);
  if (sizei < 0) {
    return ReadFile_error_ftell_error;
  }

  u32 size = Cast(u32, sizei);

  u8 *data = arena_push_array(u8, arena, size);

  fseek(f, 0, SEEK_SET);

  u64 bytes_read = fread(data, 1, size, f);
  if (bytes_read != size) {
    arena_scope_end(arena, savepoint);
    return ReadFile_error_unexpected_content_size;
  }

  *content = (String){
    .str = data,
    .len = size,
  };

  return ReadFile_ok;
}

extern b32 tokenize(Messages *messages, Tokens *tokens, String source);
extern b32 parse(Messages *messages, AstNodes *nodes, Tokens *tokens);

internal void write_tokens(Tokens *tokens, String source) {
  for (u32 i = tokens_begin(tokens); i < tokens_end(tokens); i++) {
    u8      kind = *tokens_kind(tokens, i);
    SpanU32 span = *tokens_span(tokens, i);

    char const *s = source.str + span.start;
    int len = span.end - span.start;

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
  err = parse_cli_options(&cli, argc, argv);
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
    .map_allocator = cstd_allocator,
    .scratch       = &arena_tmp,
  });

  ValueStore values = {0};
  values_init(&values, &(ValueStoreOptions){
    .payload_allocator = cstd_allocator,
  });

  Messages messages;
  messages_init(&messages);

  String source;
  err = read_file(cli.source_filename, &arena, &source);
  if (err) {
    printf("Could not read source file.\n");
    return 1;
  }

  ok = tokenize(&messages, &tokens, source);
  if (!ok) {
    messages_print_all_messages(&messages);
    return 1;
  }

  write_tokens(&tokens, source);

  ok = parse(&messages, &nodes, &tokens);
  if (!ok) {
    messages_print_all_messages(&messages);
    return 1;
  }

  write_nodes(&nodes, &tokens, source);

  printf("ok\n");

  return 0;
}
