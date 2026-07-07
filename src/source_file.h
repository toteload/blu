#ifndef SOURCE_FILE_H
#define SOURCE_FILE_H

#include "blu.h"
#include "tokens.h"
#include "ast.h"
#include "messages.h"

struct Source {
  SourceIndex idx; // Saves its own index :)
  Arena       arena;
  Arena       scratch;

  // The filename, text (source code / file contents), messages, tokens are all stored
  // in the arena of this source.
  // TODO: refactor AstNodes to also use the arena as backing memory.
  String   filename;
  String   text;
  Messages messages;
  Tokens   tokens;
  AstNodes ast;

  // - list of declarations
  //   - name of declaration
  //   - instruction index to code generated
  // - ir generated for this source
};

#define SourceList_min_size_log2  4
#define SourceList_segment_count  20
#define SEGMENTLIST_NAME          SourceList
#define SEGMENTLIST_TYPE          Source
#define SEGMENTLIST_MIN_SIZE_LOG2 SourceList_min_size_log2
#define SEGMENTLIST_SEGMENT_COUNT SourceList_segment_count
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

struct SourceAllocator {
  Arena      arena;
  SourceList list;
};

void sources_init(SourceAllocator *allocator);
void sources_deinit(SourceAllocator *allocator);

SourceIndex  sources_alloc(SourceAllocator *allocator);
Source      *sources_get(SourceAllocator *allocator, SourceIndex idx);
SourceIndex  sources_alloc_and_get(SourceAllocator *allocator, Source **source);

void source_file_init(Source *source, String filename);
void source_file_deinit(Source *source);

void error(Source *source, MessageLocation location, String format, ...);

b32 source_read_file(Source *source);
b32 source_tokenize(Source *source);
b32 source_parse(Source *source);

void source_print_all_messages(Source *source);

#endif // SOURCE_FILE_H
