#include "cli_options.h"

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
