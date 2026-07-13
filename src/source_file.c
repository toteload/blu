#include "source_file.h"
#include <stdarg.h>
#include <stdio.h>

#define SEGMENTLIST_NAME            MessageList
#define SEGMENTLIST_TYPE            MessagePtr
#define SEGMENTLIST_FUNCTION_PREFIX msglist
#define SEGMENTLIST_MIN_SIZE_LOG2   6
#define SEGMENTLIST_SEGMENT_COUNT   24
#define SEGMENTLIST_LINKAGE         internal
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

internal void source_add_message(void *user, u8 severity, MessageLocation location, String format, ...) {
  Source *source = user;

  u32 arg_count = message_format_arg_count(format);

  Message *msg = arena_push(&source->arena, sizeof(Message) + arg_count * sizeof(MessageArg), Align_of(Message));

  msg->severity = severity;
  msg->source   = source->idx;
  msg->location = location;
  msg->format   = arena_copy_string(&source->arena, format);

  va_list vl;
  va_start(vl, format);

  for (u32 i = 0; i < arg_count; i++) {
    msg->args[i] = va_arg(vl, MessageArg);
  }

  va_end(vl);

  msglist_append(&source->msg_list, &source->arena, msg);
}

void source_file_init(Source *source, SourceIndex idx, String filename) {
  zero_struct(Source, source);
  arena_init(&source->arena, &(ArenaOptions){
    .reserve_size        = MiB(64),
    .initial_commit_size = MiB(1),
  });
  arena_init(&source->scratch, &(ArenaOptions){
    .reserve_size        = MiB(4),
    .initial_commit_size = MiB(1),
  });
  source->idx      = idx;
  source->filename = arena_copy_string(&source->arena, filename);
  source->msg_sink = (MessageSink){
    .user        = source,
    .add_message = source_add_message,
  };
  nodes_init(&source->ast);
}

void source_file_deinit(Source *source) {
  arena_deinit(&source->arena);
  arena_deinit(&source->scratch);
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
    Message_error(
      &source->msg_sink,
      (MessageLocation){ .kind = MessageLocation_unspecified },
      string_lit("Could not open/read file {str}."), source->filename
    );
    return False;
  }

  return True;
}

b32 source_tokenize(Source *source) {
  TokenizeContext context = {
    .msg_sink = &source->msg_sink,
    .arena    = &source->arena,
    .scratch  = &source->scratch,
  };

  return tokenize(&context, source->text, &source->tokens);
}

#if 0
typedef struct {
  u32 decltree_idx;
  u32 idx;
  AstIndex item;
} AstWalker;

void source_index_declarations(Source *source) {
  AstNodes *ast = &source->ast;
  AstIndex at = nodes_begin(ast);

  Assert(*nodes_kind(ast, at) == Ast_source);

  NodeIndexList *items = &nodes_data(ast, at)->source.items;

  SourceDeclaration *decls = arena_push_one(SourceDeclaration, &source->arena);
  *decls = (SourceDeclaration){
    .kind = SourceDeclaration_root,
    .name = {0},
    .child_count = items->len,
    .node = 0,
  };

  for (u32 i = 0; i < items->len; i++) {
    NodeIndex item = nodelist_at(items, i);

    Assert(*nodes_kind(ast, item) == Ast_mod_section);

    AstModSection *mod_section = &nodes_data(ast, item)->mod_section;

    SourceDeclaration *mod_decl = arena_push_one(SourceDeclaration, &source->arena);
    *mod_decl = (SourceDeclaration){
      .kind = SourceDeclaration_mod,
      .name = 0, // TODO
      .child_count = mod_section->items.len,
      .node = item,
    };

    for (u32 j = 0; j < mod_section->items.len; j++) {
      NodeIndex decl = nodelist_at(mod_decl->items, j);

      Assert(*nodes_kind(ast, decl) == Ast_declaration);

      *arena_push_one(SourceDeclaration, &source->arena) = (SourceDeclaration){
        .kind = SourceDeclaration_declaration,
        .name = 0, // TODO
        .child_count = 0,
        .node = decl,
      };
    }
  }

  source->decls = decls;
}
#endif

void source_print_all_messages(Source *source) {
  u32 count = source->msg_list.len;
  for (u32 i = 0; i < count; i++) {
    Message *msg = msglist_at_unchecked(&source->msg_list, i);
    print_message(msg, source);
  }
}
