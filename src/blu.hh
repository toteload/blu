#pragma once

#include "toteload.hh"
#include "vector.hh"
#include "hashmap.hh"

#include "string_interner.hh"
#include "env.hh"

#include <stdio.h>

// -[ Source location ]-

struct SourceLocation {
  u32 line = 0;
  u32 col  = 0;
};

struct SourceSpan {
  SourceLocation start;
  SourceLocation end;

  static SourceSpan from_single_location(SourceLocation loc) {
    return {
      loc,
      {
        loc.line,
        loc.col + 1,
      }
    };
  }
};

// -[ Message ]-

enum MessageSeverity : u8 {
  Error,
  Warning,
  Info,
};

struct Message {
  SourceSpan span;
  MessageSeverity severity;
  Str message;

  char const *file = nullptr;
  u32 line         = 0;
};

#include "types.hh"

#include "tokens.hh"

struct TokenizeContext {
  Arena *arena;
  Vector<Message> *messages;
};

struct TokenizeOutput {
  Vector<TokenKind> *out;
  Vector<SourceSpan> *spans;
  Vector<Str> *strs;
};

b32 tokenize(TokenizeContext ctx, Str source, TokenizeOutput output);

#include "ast.hh"

#define ForEachAstNode(i, n) for (AstNode *i = n; i; i = i->next)

struct ParseContext {
  Arena *arena;
  Vector<Message> *messages;

  StringInterner *strings;
  ObjectPool<AstNode> *nodes;
};

b32 parse(ParseContext ctx, TokenizeOutput tokens, AstNode **root);

struct TypeCheckContext {
  Arena *arena;
  Vector<Message> *messages;

  EnvManager *envs;
  TypeInterner *types;
  ObjectPool<AstNode> *nodes;
};

b32 infer_types(TypeCheckContext ctx, AstNode *root);

//b32 generate_c_code(CompilerContext *ctx, FILE *out, AstNode *mod);

// -[ Compiler context ]-

struct SourceFile {
  Str filename;
  Str source;

  Vector<TokenKind> tokens;
  Vector<SourceSpan> spans;
  Vector<Str> strs;

  AstNode *root = nullptr;
};

struct CompilerContext {
  Arena arena;
  Arena work_arena;

  Vector<Message> messages;

  StringInterner strings;
  TypeInterner types;
  ObjectPool<AstNode> nodes;
  EnvManager environments;
  Env *global_environment;

  Vector<SourceFile> sources;

  // ---

  void init();
  b32 add_source_file_compile_job(Str filename);
};

// clang-format off
//#define Push_message(Messages, Msg) { Message _tmp = Msg; _tmp.file = __FILE__; _tmp.line = __LINE__; (Messages).push(_tmp); }
// clang-format on
