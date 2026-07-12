#ifndef COMPILER_H
#define COMPILER_H

#include "blu.h"
#include "messages.h"

// - For each source file:
//   - read file, tokenize, parse
//   - output list of decls
// - Merge the list of decls into one map.
//   At this point we have all the necessary information to resolve all the identifiers.
//   !!! THIS IS NOT (entirely?) TRUE !!!
//   You could have a declaration for a comptime function that returns a module.
//   The contents of this module are arbitrary and are only known after evaluating the function.
//   Actually, I think this works out fine.
// - For each source file:
//   - Resolve all the identifiers.
//   - Generate code
//   - Output dependency information for each chunk of code.
// - Now, each source file will have generated code and dependency information.
//   Use this information to compute a dependency graph and walk the graph.

#define SourceList_min_size_log2  4
#define SourceList_segment_count  20
#define SEGMENTLIST_NAME          SourceList
#define SEGMENTLIST_TYPE          Source
#define SEGMENTLIST_MIN_SIZE_LOG2 SourceList_min_size_log2
#define SEGMENTLIST_SEGMENT_COUNT SourceList_segment_count
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

typedef struct {
  Arena arena;
  Arena scratch;

  SourceList  sources;
  MessageSink msg_sink;
} Compiler;

void compiler_init(Compiler *compiler);
void compiler_deinit(Compiler *compiler);

void compiler_add_sourcefile(Compiler *compiler, String filename);

void compiler_print_all_messages(Compiler *compiler);

b32 compile(Compiler *compiler);

#endif // COMPILER_H
