#pragma once

#include "toteload.hh"
#include "vector.hh"
#include "hashmap.hh"

#include "string_interner.hh"

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

struct Token {
  TokenKind kind;
  SourceSpan span;
  Str str;
};

struct TokenizeContext {
  Arena *arena;
  Vector<Message> *messages;
};

// TODO: Convert tokenize output of tokens to SoA layout

b32 tokenize(TokenizeContext ctx, Str source, Vector<Token> *output);

#include "ast.hh"

ttld_inline Str get_ast_str(AstNode *node, Slice<Token> tokens) {
  char const *start = tokens[node->token_span.start].str.str;
  char const *end = tokens[node->token_span.end].str.end();
  return { start, cast<usize>(end - start), };
}

ttld_inline SourceSpan get_ast_source_span(AstNode *node, Slice<Token> tokens) {
  SourceLocation start = tokens[node->token_span.start].span.start;
  SourceLocation end = tokens[node->token_span.end].span.end;
  return { start, end, };
}

#define ForEachAstNode(i, n) for (AstNode *i = n; i; i = i->next)

struct ParseContext {
  Arena *arena;
  Vector<Message> *messages;

  StringInterner *strings;
  ObjectPool<AstNode> *nodes;
};

b32 parse(ParseContext ctx, Slice<Token> tokens, AstNode **root);

#include "value.hh"
#include "env.hh"

struct TypeCheckContext {
  Arena *arena;
  Vector<Message> *messages;

  Arena *work_arena;

  EnvManager *envs;
  TypeInterner *types;
  ObjectPool<AstNode> *nodes;
};

b32 type_check_module(TypeCheckContext ctx, AstNode *module);

struct CCodeGenerateContext {
  Arena *work_arena;

};

b32 generate_c_code(CCodeGenerateContext *ctx, AstNode *mod);

// -[ Compiler context ]-

enum JobKind : u8 {
  Job_read_file,
  Job_tokenize,
  Job_parse,
  Job_typecheck,
};

typedef u32 SourceIdx;

struct Job {
  JobKind kind;
  SourceIdx source_idx;
};

struct Source {
  Str filename;

  Str text;
  Vector<Token> tokens;
  AstNode *mod = nullptr;
};

struct Compiler {
  Arena arena;
  Arena work_arena;

  Vector<Message> messages;

  StringInterner strings;
  TypeInterner types;
  ObjectPool<AstNode> nodes;
  EnvManager environments;

  Vector<Source> sources;
  Queue<Job> jobs;

  // ---

  void init();
  b32 add_source_file_compile_job(Str filename);

  void run();
};

// clang-format off
//#define Push_message(Messages, Msg) { Message _tmp = Msg; _tmp.file = __FILE__; _tmp.line = __LINE__; (Messages).push(_tmp); }
// clang-format on
