#ifndef SOURCE_FILE_H
#define SOURCE_FILE_H

#include "blu.h"
#include "tokens.h"
#include "ast.h"
#include "messages.h"

#if 0
// TODO remove
// just a sketch
struct Declaration {
  u8 kind; // param | local | builtin | decl
  
  union {
    AstIndex param;
    AstIndex local;
    ValueIndex builtin;
    struct {
      SourceIndex source;
      AstIndex    ast;
    } decl;
  } data;
};

struct ModuleLevelDeclaration {
  AstIndex ast_index;
  String name;
};

struct DeclarationKey {
  StringIndex parent;
  StringIndex name;
};

struct DeclarationValue {
  u8 kind; // mod | decl | builtin

  union {
    ModuleIndex mod;
    ValueIndex builtin;
  } data;
};

// root
// |
// |- mod main
// |  \- fn main
// |
// \- mod util
//    \- fn print

enum SourceDeclarationKind {
  SourceDeclaration_mod,
  SourceDeclaration_declaration,
};

typedef struct {
  u8       kind;
  String   name;
  u32      tree_size;
  AstIndex node;
} SourceDeclaration;
#endif

typedef Message* MessagePtr;

#define SEGMENTLIST_NAME          MessageList
#define SEGMENTLIST_TYPE          MessagePtr
#define SEGMENTLIST_MIN_SIZE_LOG2 6
#define SEGMENTLIST_SEGMENT_COUNT 24
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

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

  // The filename, text (source code / file contents), messages, tokens are all stored
  // in the arena of this source.
  // TODO: refactor AstNodes to also use the arena as backing memory.
  String   filename;
  String   text;
  Tokens   tokens;
  AstNodes ast;

  u32 source_decl_count;
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
void source_list_decls(Source *source);

void source_print_all_messages(Source *source);

#endif // SOURCE_FILE_H
