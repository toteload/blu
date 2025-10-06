#include <stdio.h>

#include "blu.hh"
#include "utils/stdlib.hh"

int main(i32 arg_count, char const *const *args) {
  if (arg_count < 2) {
    printf("Please provide an input file\n");
    return 1;
  }

  Str filename = Str::from_cstr(args[1]);

  Str source_text = read_file(filename);

  Arena arena;
  arena.init(MiB(2));

  Arena work_arena;
  work_arena.init(MiB(1));

  TypeInterner types;
  types.init(&work_arena, stdlib_alloc, stdlib_alloc, stdlib_alloc);

  StringInterner strings;
  strings.init(arena.as_allocator(), stdlib_alloc, stdlib_alloc);

  MessageManager messages;
  messages.init(stdlib_alloc, &strings, &types);

  Tokens tokens;
  tokens.kinds.init(stdlib_alloc);
  tokens.spans.init(stdlib_alloc);

  b32 ok;
  ok = tokenize(&messages, source_text, &tokens);

  if (!ok) {
    printf("Tokenization error\n");
    return 1;
  }

  AstNodes nodes;
  nodes.kinds.init(stdlib_alloc);
  nodes.spans.init(stdlib_alloc);
  nodes.datas.init(stdlib_alloc);
  nodes.segment_allocator = arena.as_allocator();

  ParseContext parse_context;
  parse_context.messages = &messages;
  parse_context.tokens   = &tokens;

  ok = parse(&parse_context, &nodes);
  if (!ok) {
    printf("Parse error\n");
    return 1;
  }

  EnvManager envs;
  envs.init(arena.as_allocator(), stdlib_alloc);
  envs.init_global_env(&strings, &types);

  ValueStore values;
  values.init(stdlib_alloc);

  TypeCheckContext type_check_context;
  type_check_context.messages   = &messages;
  type_check_context.work_arena = &work_arena;
  type_check_context.envs       = &envs;
  type_check_context.types      = &types;
  type_check_context.strings    = &strings;
  type_check_context.values = &values;

  Source source;
  source.filename = filename;
  source.source   = source_text;
  source.tokens   = &tokens;
  source.nodes    = &nodes;

  ok = type_check(&type_check_context, &source);
  if (!ok) {
    printf("Type check error\n");
    messages.print_messages();
    return 1;
  }

  printf("ok\n");

  return 0;
}
