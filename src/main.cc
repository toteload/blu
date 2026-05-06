#include <stdio.h>

#include "blu.hh"
#include "utils/stdlib.hh"

struct CLISettings {
  bool verbose;
  Str source_file;
};

b32 parse_cli_settings(CLISettings *settings, i32 arg_count, char const *const *args) {
  *settings = {
    .verbose     = false,
    .source_file = Str::empty(),
  };

  for (i32 i = 1; i < arg_count; i++) {
    Str arg = Str::from_cstr(args[i]);
    if (str_eq(arg, STR("-v"))) {
      settings->verbose = true;
    } else if (settings->source_file.is_empty()) {
      settings->source_file = arg;
    } else {
      printf("Unexpected argument: %s\n", args[i]);
      return false;
    }
  }

  if (settings->source_file.is_empty()) {
    printf("Please provide an input file\n");
    return false;
  }

  return true;
}

int main(i32 arg_count, char const *const *args) {
  CLISettings settings;
  if (!parse_cli_settings(&settings, arg_count, args)) {
    return 1;
  }

  Str filename    = settings.source_file;
  Str source_text = read_file(filename);

  Arena arena{};
  arena.init(MiB(2));

  Arena work_arena{};
  work_arena.init(MiB(8));

  TypeInterner types{};
  types.init(&work_arena, stdlib_alloc, stdlib_alloc, stdlib_alloc);

  StringInterner strings{};
  strings.init(arena.as_allocator(), stdlib_alloc, stdlib_alloc);

  Source source{};
  source.filename = filename;
  source.source   = source_text;

  Messages messages{};
  messages.source = &source;
  messages.init(stdlib_alloc, &strings, &types);

  Tokens tokens{};
  tokens.kinds.init(stdlib_alloc);
  tokens.spans.init(stdlib_alloc);

  b32 ok;
  ok = tokenize(&messages, source_text, &tokens);

  source.tokens = &tokens;

  if (!ok) {
    printf("Tokenization error\n");
    return 1;
  }

  if (settings.verbose) {
    auto snapshot = work_arena.take_snapshot();

    write_tokens(&tokens, source_text, &work_arena);
    printf("%.*s", (int)snapshot.size(), (char *)snapshot.at);

    work_arena.restore(snapshot);
  }

  AstNodes nodes;
  nodes.kinds.init(stdlib_alloc);
  nodes.spans.init(stdlib_alloc);
  nodes.datas.init(stdlib_alloc);
  nodes.segment_allocator = arena.as_allocator();

  ParseContext parse_context;
  parse_context.messages = &messages;
  parse_context.tokens   = &tokens;

  ok = parse(&parse_context, source_text, &nodes);

  source.nodes = &nodes;

  if (!ok) {
    printf("Parse error\n");
    messages.print_messages();
    return 1;
  }

  if (settings.verbose) {
    for (u32 i = 0; i < nodes.kinds.len(); i++) {
      printf("%s\n", ast_kind_string(nodes.kinds[i]));
    }
  }

  ValueStore values;
  values.init(stdlib_alloc);

  EnvManager<Declaration> envs;
  envs.init(stdlib_alloc, stdlib_alloc);

  TypeCheckContext typecheck_context = {
    .messages   = &messages,
    .envs       = &envs,
    .types      = &types,
    .strings    = &strings,
    .work_arena = &work_arena,
  };

  Vector<TypeIndex> type_annotations;
  type_annotations.init(stdlib_alloc);
  type_annotations.set_size(nodes.kinds.len());
  memset(type_annotations.data, 0xff, nodes.kinds.len() * sizeof(TypeIndex));

  ok = typecheck(&typecheck_context, &source, type_annotations.slice());
  if (!ok) {
    printf("Typecheck error\n");
    messages.print_messages();
    return 1;
  }

  EnvManager<ValueIndex> value_envs;
  value_envs.init(stdlib_alloc, stdlib_alloc);

  Interpreter interpreter;
  interpreter.init(&strings, &types, &values, &value_envs, &work_arena, &messages);
  interpreter.type_annotations = type_annotations.slice();

  ValueIndex result;
  ok = interpreter.run(&source, &result);
  if (!ok) {
    printf("Interpreter error encountered.\n");
    messages.print_messages();
    return 1;
  }

  {
    char buf[512] = {0};
    u32 len       = values.value_to_string(&types, result, buf, 512);
    printf("%.*s\n", cast<int>(len), buf);
  }

  return 0;
}
