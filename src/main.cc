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

  SourceUnit unit{};
  unit.init(filename, source_text);
  defer(unit.deinit());

  bool ok;
  ok = unit.tokenize();
  if (!ok) {
    printf("Tokenize error\n");
    unit.print_messages();
    return 1;
  }

  //  if (settings.verbose) {
  //    auto snapshot = work_arena.take_snapshot();
  //
  //    write_tokens(&tokens, source_text, &work_arena);
  //    printf("%.*s", (int)snapshot.size(), (char *)snapshot.at);
  //
  //    work_arena.restore(snapshot);
  //  }

  ok = unit.parse();
  if (!ok) {
    printf("Parse error\n");
    unit.print_messages();
    return 1;
  }

  ok = unit.typecheck();
  if (!ok) {
    printf("Typecheck error\n");
    unit.print_messages();
    return 1;
  }

  ok = unit.run_const_code();
  if (!ok) {
    printf("Const code evaluation error\n");
    unit.print_messages();
    return 1;
  }

  if (settings.verbose) {
    ast_pretty_print(
      unit.text,
      &unit.tokens,
      &unit.types,
      &unit.interpreter.values,
      &unit.nodes,
      unit.nodes.first_valid_index()
    );
  }

  ValueIndex result;
  ok = unit.run_main(&result);
  if (!ok) {
    printf("Runtime error\n");
    unit.print_messages();
    return 1;
  }

  i32 result_code = *cast<i32 *>(unit.interpreter.values.get(result)->data);
  return result_code;
}
