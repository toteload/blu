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
#endif

enum SourceDeclarationKind {
  SourceDeclaration_mod,
  SourceDeclaration_declaration,
};

typedef struct {
  u8 kind;
  String name;
  u32 tree_size;
} SourceDeclaration;

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

  u32 source_decl_count;
  SourceDeclaration *decls;

  // - list of declarations
  //   - name of declaration
  //   - instruction index to code generated
  // - ir generated for this source
};

#if 0
typedef u32 ModuleIndex;

struct DeclarationKey {
  ModuleIndex parent;
  StringIndex name;
};

struct DeclarationValue {
  u8 kind; // mod | decl | builtin

  union {
    ModuleIndex mod;
    ValueIndex builtin;
  } data;
};
#endif

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
// - At this point each source file will have generated code and dependency information.
//   Use this information to compute a dependency graph and walk the graph.

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
void source_list_decls(Source *source);

void source_print_all_messages(Source *source);

typedef struct {
  SourceAllocator sources;
} Compiler;

void compiler_init(Compiler *compiler);
void compiler_deinit(Compiler *compiler);

void compiler_add_sourcefile(Compiler *compiler, String filename);

u32 compile(Compiler *compiler);

#endif // SOURCE_FILE_H
