#include "source_file.h"
#include <stdarg.h>
#include <stdio.h>

#define SEGMENTLIST_NAME            SourceList
#define SEGMENTLIST_TYPE            Source
#define SEGMENTLIST_FUNCTION_PREFIX list
#define SEGMENTLIST_MIN_SIZE_LOG2   SourceList_min_size_log2
#define SEGMENTLIST_SEGMENT_COUNT   SourceList_segment_count
#define SEGMENTLIST_LINKAGE         internal
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

void sources_init(SourceAllocator *sources) {
  zero_struct(SourceAllocator, sources);
  arena_init(&sources->arena, &(ArenaOptions){
    .reserve_size        = MiB(1),
    .initial_commit_size = KiB(4),
  });
  list_push(&sources->list, &sources->arena);
}

void sources_deinit(SourceAllocator *sources) {
  arena_deinit(&sources->arena);
  zero_struct(SourceAllocator, sources);
}

SourceIndex sources_alloc(SourceAllocator *sources) {
  SourceIndex idx = sources->list.len;
  Source *s = list_push(&sources->list, &sources->arena);
  s->idx = idx;
  return idx;
}

Source *sources_get(SourceAllocator *sources, SourceIndex idx) {
  return list_ptr_at_unchecked(&sources->list, idx);
}

SourceIndex sources_alloc_and_get(SourceAllocator *sources, Source **source) {
  SourceIndex idx = sources_alloc(sources);
  *source = sources_get(sources, idx);
  return idx;
}

void source_file_init(Source *source, String filename) {
  arena_init(&source->arena, &(ArenaOptions){
    .reserve_size        = MiB(64),
    .initial_commit_size = MiB(1),
  });
  arena_init(&source->scratch, &(ArenaOptions){
    .reserve_size        = MiB(4),
    .initial_commit_size = MiB(1),
  });
  source->filename = arena_copy_string(&source->arena, filename);
  zero_struct(String, &source->text);
  messages_init(&source->messages);
  tokens_init(&source->tokens, &source->arena);
  nodes_init(&source->ast);
}

void source_file_deinit(Source *source) {
  arena_deinit(&source->arena);
  arena_deinit(&source->scratch);
}

void error(Source *source, MessageLocation location, String format, ...) {
  va_list vl;
  va_start(vl, format);
  messages_errorv(&source->messages, &source->arena, source->idx, location, format, vl);
  va_end(vl);
}

enum ReadFileResult {
  ReadFile_ok,
  ReadFile_error_could_not_open_file,
  ReadFile_error_unexpected_content_size,
  ReadFile_error_ftell_error,
};

// `read_file` reads the contents of the given filename and stores it in the provided arena.
internal u32 read_file(String filename, Arena *arena, String *content) {
  FILE *f;
  {
    ArenaSnapshot scope = arena_scope_begin(arena);
    u8 *buf = arena_push_array(u8, arena, filename.len + 1);
    memcpy(buf, filename.str, filename.len);
    buf[filename.len] = '\0';

    f = fopen(Cast(char const *, buf), "rb");
    if (is_null(f)) {
      arena_scope_end(arena, scope);
      return ReadFile_error_could_not_open_file;
    }

    arena_scope_end(arena, scope);
  }

  fseek(f, 0, SEEK_END);

  ArenaSnapshot savepoint = arena_scope_begin(arena);

  i32 sizei = ftell(f);
  if (sizei < 0) {
    return ReadFile_error_ftell_error;
  }

  u32 size = Cast(u32, sizei);

  u8 *data = arena_push_array(u8, arena, size);

  fseek(f, 0, SEEK_SET);

  u64 bytes_read = fread(data, 1, size, f);
  if (bytes_read != size) {
    arena_scope_end(arena, savepoint);
    return ReadFile_error_unexpected_content_size;
  }

  *content = (String){
    .str = data,
    .len = size,
  };

  return ReadFile_ok;
}

b32 source_read_file(Source *source) {
  u32 err = read_file(source->filename, &source->arena, &source->text);
  if (err) {
    error(
      source,
      (MessageLocation){ .kind = MessageLocation_unspecified },
      string_lit("Could not open/read file {str}."), source->filename
    );
    return False;
  }

  return True;
}

#if 0
void source_list_decls(Source *source) {
  AstNodes *ast = &source->ast;
  AstIndex at = nodes_begin(ast);
  Assert(*nodes_kind(ast) == Ast_source);

  AstSource *s = nodes_data(ast, at);

  for (u32 i = 0; i < s->items.len; i++) {
    AstIndex item = *astlist_ptr_at_unchecked(&s->items, i);
  }
}
#endif

void source_print_all_messages(Source *source) {
  u32 count = messages_count(&source->messages);
  for (u32 i = 0; i < count; i++) {
    Message *msg = messages_get(&source->messages, i);
    print_message(msg, source);
  }
}
