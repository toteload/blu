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
  work_arena.init(MiB(8));

  TypeInterner types;
  types.init(&work_arena, stdlib_alloc, stdlib_alloc, stdlib_alloc);

  StringInterner strings;
  strings.init(arena.as_allocator(), stdlib_alloc, stdlib_alloc);

  Source source;
  source.filename = filename;
  source.source   = source_text;

  Messages messages;
  messages.init(stdlib_alloc, &strings, &types, source_text);

  Tokens tokens;
  tokens.kinds.init(stdlib_alloc);
  tokens.spans.init(stdlib_alloc);

  b32 ok;
  ok = tokenize(&messages, source_text, &tokens);

  if (!ok) {
    printf("Tokenization error\n");
    return 1;
  }

  {
    auto snapshot = work_arena.take_snapshot();

    write_tokens(&tokens, source_text, &work_arena);
    printf("%.*s", (int)snapshot.size(), (char *)snapshot.at);

    work_arena.restore(snapshot);
  }

  source.tokens   = &tokens;

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
  
  source.nodes    = &nodes;

  ValueStore values;
  values.init();

  EnvManager envs;
  envs.init(stdlib_alloc, stdlib_alloc);

  Interpreter interpreter;
  interpreter.init(&strings, &types, &values, &envs, &work_arena, &messages);

  ValueIndex result;
  ok = interpreter.run(&source, &result);
  if (!ok) {
    printf("Interpreter error encountered.\n");
    return 1;
  }

  {
    char buf[512] = {0};
    u32 len       = values.value_to_string(&types, result, buf, 512);
    printf("%.*s\n", cast<int>(len), buf);
  }

  printf("ok\n");

  return 0;
}
