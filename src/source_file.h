#ifndef SOURCE_FILE_H
#define SOURCE_FILE_H

#include "blu.h"
#include "tokens.h"
#include "ast.h"
#include "messages.h"
#include "string_interner.h"
#include "ir.h"

enum SourceDeclarationKind {
  SourceDeclaration_root,
  SourceDeclaration_mod,
  SourceDeclaration_declaration,
};

typedef struct {
  u8          kind;
  AstIndex    node;
  u32         child_count;
  u32         parent;
  StringIndex name;
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

  MessageList msg_list;
  MessageSink msg_sink;

  // The filename, text (source code / file contents), messages, tokens and AST are all stored
  // in the arena of this source.
  String   filename;
  String   text;
  Tokens   tokens;
  AstNodes ast;

  u32                decl_tree_size;
  SourceDeclaration *decls;
  DeclarationIndex  *decl_idxs;

  u32      decl_count;
  u32     *tree_idxs;
  IrChunk *ir_chunks;
  IrChunk *runtime_chunks;
};

void source_file_init(Source *source, SourceIndex idx, String filename);
void source_file_deinit(Source *source);

b32 source_read_file(Source *source);
b32 source_tokenize(Source *source, Arena *scratch);
b32 source_parse(Source *source, Arena *scratch);

// - Creates the declaration tree for this source file and stores it in `decls`.
// - Writes the size of the the tree in `decl_tree_size` (the number of SourceDeclarations).
// - Writes the number of declarations in `decl_count` (`decl_tree_size` - (number of module declarations)).
// - Allocates `decl_idxs` (decl_tree_size).
// - Allocates `tree_idx` (decl_count).
// - Allocates `ir_chunks` (decl_count).
// - Allocates `runtime_chunks` (decl_count).
void source_index_declarations(Source *source, StringInterner *strings);

void source_print_all_messages(Source *source);

#endif // SOURCE_FILE_H
