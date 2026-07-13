#ifndef CLI_OPTIONS_H
#define CLI_OPTIONS_H

#include "toteload.h"

typedef struct {
  b32 verbose;
  String source_filename;
} CLIOptions;

enum ParseCliResult {
  ParseCli_ok,
  ParseCli_error_unknown_argument,
  ParseCli_error_no_source_filename_provided,
};

u32 parse_cli_options(CLIOptions *options, u32 arg_count, char const *const *args);

#endif // CLI_OPTIONS_H
