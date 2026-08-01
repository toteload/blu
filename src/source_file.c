#include "source_file.h"
#include "messages.h"
#include <stdarg.h>
#include <stdio.h>

internal void source_add_message(void *user, u8 severity, MessageLocation location, String format, ...) {
  Source *source = user;

  u32 arg_count = message_format_arg_count(format);

  Message *msg = arena_push(&source->arena, sizeof(Message) + arg_count * sizeof(MessageArg), Align_of(Message));

  msg->severity = severity;
  msg->location = location;
  msg->location.source_idx = source->idx;
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
  source->idx      = idx;
  source->filename = arena_copy_string(&source->arena, filename);
  source->msg_sink = (MessageSink){
    .user        = source,
    .add_message = source_add_message,
  };
}

void source_file_deinit(Source *source) {
  arena_deinit(&source->arena);
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

  i32 sizei = ftell(f);
  if (sizei < 0) {
    fclose(f);
    return ReadFile_error_ftell_error;
  }

  u32 size = Cast(u32, sizei);

  ArenaSnapshot savepoint = arena_scope_begin(arena);

  u8 *data = arena_push_array(u8, arena, size);

  fseek(f, 0, SEEK_SET);

  u64 bytes_read = fread(data, 1, size, f);

  fclose(f);

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

b32 source_tokenize(Source *source, Arena *scratch) {
  TokenizeContext context = {
    .msg_sink = &source->msg_sink,
    .arena    = &source->arena,
    .scratch  = scratch,
  };

  return tokenize(&context, source->text, &source->tokens);
}

b32 source_parse(Source *source, Arena *scratch) {
  ParseContext context = {
    .msg_sink = &source->msg_sink,
    .arena    = &source->arena,
    .scratch  = scratch,
  };

  return parse(&context, &source->tokens, &source->ast);
}

void source_index_declarations(Source *source, StringInterner *strings) {
  AstNodes *ast = &source->ast;
  Tokens *tokens = &source->tokens;
  String text = source->text;

  Assert(ast->kinds[AstIndex_source] == Ast_source);

  AstSource *s = ast_data(ast, AstIndex_source);

  SourceDeclaration *decls = arena_push_one(SourceDeclaration, &source->arena);
  *decls = (SourceDeclaration){
    .kind = SourceDeclaration_root,
    .name = 0,
    .child_count = s->count,
    .parent = 0,
    .node = 0,
  };

  u32 count = 1;

  for (u32 i = 0; i < s->count; i++) {
    AstIndex item = s->items[i];

    Assert(ast->kinds[item] == Ast_mod_section);

    AstModSection *mod_section = ast_data(ast, item);

    String mod_name = token_string(tokens, text, mod_section->name);
    StringIndex name = strings_add(strings, mod_name);

    u32 mod_index = count;

    SourceDeclaration *mod_decl = arena_push_one(SourceDeclaration, &source->arena);
    *mod_decl = (SourceDeclaration){
      .kind = SourceDeclaration_mod,
      .name = name,
      .child_count = mod_section->count,
      .parent = 0,
      .node = item,
    };

    count += 1 + mod_section->count;

    for (u32 j = 0; j < mod_section->count; j++) {
      AstIndex decl_idx = mod_section->items[j];

      Assert(ast->kinds[decl_idx] == Ast_declaration);

      AstDeclaration *ast_decl = ast_data(ast, decl_idx);

      String decl_name = token_string(tokens, text, ast_decl->name);
      StringIndex name = strings_add(strings, decl_name);

      SourceDeclaration *decl = arena_push_one(SourceDeclaration, &source->arena);
      *decl = (SourceDeclaration){
        .kind = SourceDeclaration_declaration,
        .name = name,
        .child_count = 0,
        .parent = mod_index,
        .node = decl_idx,
      };
    }
  }

  source->decl_tree_size = count;
  source->decls          = decls;
  source->decl_idxs      = arena_push_array(DeclarationIndex, &source->arena, count);
}

void source_print_all_messages(Source *source) {
  u32 count = source->msg_list.len;
  for (u32 i = 0; i < count; i++) {
    Message *msg = msglist_at_unchecked(&source->msg_list, i);
    print_message(msg, source, Null);
  }
}
