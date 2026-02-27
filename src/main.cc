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

  for (u32 i = 0; i < tokens.kinds.len(); i++) {
    printf("%s\n", token_kind_string(tokens.kinds[i]));
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
    messages.print_messages();
    return 1;
  }

  for (u32 i = 0; i < nodes.kinds.len(); i++) {
    printf("%s\n", ast_kind_string(nodes.kinds[i]));
  }

  ValueStore values;
  values.init(stdlib_alloc);

  EnvManager envs;
  envs.init(arena.as_allocator(), stdlib_alloc, &strings, &types, &values);

  Source source;
  source.filename = filename;
  source.source   = source_text;
  source.tokens   = &tokens;
  source.nodes    = &nodes;

  HirGeneratorContext hir_generator_context;
  hir_generator_context.messages = &messages;
  hir_generator_context.strings = &strings;

  HirCode hir_code;
  hir_code.kinds.init(stdlib_alloc);
  hir_code.datas.init(stdlib_alloc);
  hir_code.extra.init(stdlib_alloc);

  ok = generate_hir(&hir_generator_context, &source, &hir_code);
  if (!ok) {
    printf("HIR generation error\n");
    return 1;
  }

  printf("--- HIR ---\n");
  for (u32 i = 0; i < hir_code.kinds.len(); i++) {
    printf("%s\n", hir_kind_string(hir_code.kinds[i]));
  }

  printf("ok\n");

  return 0;
}
