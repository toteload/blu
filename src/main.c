#include "toteload.h"
#include "compiler.h"
#include "cli_options.h"

#include <stdio.h>

int main(int argc, char const *argv[]) {
  u32 err = 0;
  b32 ok = False;
  CLIOptions cli = {0};
  err = parse_cli_options(&cli, Cast(u32, argc), argv);
  if (err) {
    printf("Encountered error reading command line options.\n");
    return 1;
  }

  Compiler compiler;
  compiler_init(&compiler, &cli);

  compiler_add_sourcefile(&compiler, cli.source_filename);

  ok = compile(&compiler);

  if (!ok) {
    compiler_print_all_messages(&compiler);
    return 1;
  }

  ok = run_main(&compiler);
  if (!ok) {
    compiler_print_all_messages(&compiler);
    return 1;
  }

  return (ok) ? 0 : 1;
}
