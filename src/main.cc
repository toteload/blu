#include <stdio.h>

#include "blu.hh"
#include "utils/stdlib.hh"

struct CLISettings {
  bool verbose;
  Str  source_file;
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

  if (source_text.is_empty()) {
    printf("Invalid source file provided.\n");
    return 1;
  }

  Arena arena;
  arena.init(MiB(2));

  Arena arena_tmp;
  arena_tmp.init(MiB(2));

  Messages messages{};
  messages.init(stdlib_alloc);

  Tokens tokens{};
  tokens.init(stdlib_alloc);

  AstNodes nodes{};
  nodes.init(arena.as_allocator(), arena.as_allocator());

  StringInterner strings{};
  strings.init(arena.as_allocator(), stdlib_alloc, stdlib_alloc);

  TypeInterner types{};
  types.init(&arena_tmp, arena.as_allocator(), stdlib_alloc, stdlib_alloc);

  EnvManager<Declaration> envs{};
  envs.init(stdlib_alloc, stdlib_alloc);

  ValueStore values{};
  values.init(stdlib_alloc);

  b32 ok;
  {
    TokenizeContext context{};
    context.messages = &messages;
    ok               = tokenize(&context, source_text, &tokens);
  }
  if (!ok) {
    printf("error: tokenization\n");
    return 1;
  }

  {
    ParseContext context{};
    context.messages = &messages;
    ok               = parse_root(&context, &tokens, &nodes);
  }
  if (!ok) {
    printf("error: parsing\n");
    return 1;
  }

  AstPrettyPrintContext print_context{};
  print_context.text   = source_text;
  print_context.tokens = &tokens;
  print_context.types  = &types;
  print_context.nodes  = &nodes;
  print_context.values = &values;

  if (settings.verbose) {
    pretty_print(&print_context, Print_basic, nodes.first_valid_index());
    table_print_ast(&print_context);
  }

  Builder builder{};
  builder.envs      = &envs;
  builder.types     = &types;
  builder.strings   = &strings;
  builder.messages  = &messages;
  builder.values    = &values;
  builder.arena_tmp = &arena_tmp;
  builder.text      = source_text;
  builder.tokens    = &tokens;
  builder.nodes     = &nodes;

  builder.init();
  defer(builder.deinit());

  ok = builder.typecheck_and_eval_const_code();
  if (!ok) {
    MessageContext context;
    context.text    = source_text;
    context.tokens  = &tokens;
    context.nodes   = &nodes;
    context.types   = &types;
    context.strings = &strings;
    messages.print_messages(&context);
    return 1;
  }

  if (settings.verbose) {
    pretty_print(&print_context, Print_with_types, nodes.first_valid_index());
    table_print_ast(&print_context);

    for (auto it = builder.env_root->map.first_valid_entry(); it != builder.env_root->map.end();
         it      = builder.env_root->map.next(it)) {
      auto s = strings.get(it->key);
      char buf[256]{};
      u32  len = values.value_to_string(&types, it->val.node_index.as_value_idx(), buf, 256);
      printf("%.*s = %.*s\n", cast<int>(s.len()), s.str, cast<int>(len), buf);
    }
  }

  auto key = strings.add(STR("main"));
  Declaration decl_main;
  b32 found = builder.env_root->lookup(key, &decl_main);
  Assert(found);

  ValueIndex result;
  ok = builder.eval_call(builder.env_root, decl_main.node_index.as_value_idx(), {}, &result);
  if (!ok) {
    MessageContext context;
    context.text    = source_text;
    context.tokens  = &tokens;
    context.nodes   = &nodes;
    context.types   = &types;
    context.strings = &strings;

    messages.print_messages(&context);

    return 1;
  }

  char buf[256]{};
  u32  len = values.value_to_string(&types, result, buf, 256);
  printf("%.*s\n", cast<int>(len), buf);

  return 0;
}
