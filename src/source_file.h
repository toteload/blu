#ifndef SOURCE_FILE_H
#define SOURCE_FILE_H

#include "blu.h"
#include "tokens.h"
#include "ast.h"
#include "messages.h"

enum SourceDeclarationKind {
  SourceDeclaration_root,
  SourceDeclaration_mod,
  SourceDeclaration_declaration,
};

typedef struct {
  u8       kind;
  String   name;
  u32      child_count;
  AstIndex node;

  DeclarationIndex decl_idx;
} SourceDeclaration;

enum SourceStatus {
  SourceStatus_unprocessed,
  SourceStatus_failed_to_parse,
  SourceStatus_parsed,
};

struct Source {
  u32         status;
  SourceIndex idx; // Saves its own index :)
  Arena       arena;
  Arena       scratch;

  MessageList msg_list;
  MessageSink msg_sink;

  // The filename, text (source code / file contents), messages, tokens and AST are all stored
  // in the arena of this source.
  String   filename;
  String   text;
  Tokens   tokens;
  AstNodes ast;

  SourceDeclaration *decls;

  // - list of declarations
  //   - name of declaration
  //   - instruction index to code generated
  // - ir generated for this source
};

void source_file_init(Source *source, SourceIndex idx, String filename);
void source_file_deinit(Source *source);

b32 source_read_file(Source *source);
b32 source_tokenize(Source *source);
b32 source_parse(Source *source);
void source_index_declarations(Source *source);

void source_generate_code_for_declaration(Source *source, StringInterner *strings, AstIndex decl);

void source_print_all_messages(Source *source);

#endif // SOURCE_FILE_H
