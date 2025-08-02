#pragma once

#include "toteload.hh"
#include "vector.hh"
#include "queue.hh"
#include "hashmap.hh"

#include "string_interner.hh"

#include <stdio.h>

typedef u32 SourceIdx;

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

struct MessageManager;

#include "types.hh"

#include "tokens.hh"

struct Token {
  TokenKind kind;
  SourceSpan span;
  Str str;
};

struct TokenizeContext {
  MessageManager *messages;
};

// TODO: Convert tokenize output of tokens to SoA layout

b32 tokenize(TokenizeContext ctx, Str source, Vector<Token> *output);

#include "ast.hh"

ttld_inline Str get_ast_str(AstNode *node, Slice<Token> tokens) {
  char const *start = tokens[node->token_span.start].str.str;
  char const *end   = tokens[node->token_span.end - 1].str.end();
  return {
    start,
    cast<usize>(end - start),
  };
}

ttld_inline SourceSpan get_ast_source_span(AstNode *node, Slice<Token> tokens) {
  SourceLocation start = tokens[node->token_span.start].span.start;
  SourceLocation end   = tokens[node->token_span.end].span.end;
  return {
    start,
    end,
  };
}

#define ForEachAstNode(i, n) for (AstNode *i = n; i; i = i->next)

struct ParseContext {
  SourceIdx src_idx;
  MessageManager *messages;
  StringInterner *strings;
  ObjectPool<AstNode> *nodes;
};

b32 parse(ParseContext ctx, Slice<Token> tokens, AstNode **root);

#include "value.hh"
#include "env.hh"

struct Source {
  Str filename;

  Str text;
  Vector<Token> tokens;

  AstNode *mod = nullptr;
};

struct TypeCheckContext {
  MessageManager *messages;

  Arena *arena;
  Arena *work_arena;

  EnvManager *envs;
  TypeInterner *types;
  ObjectPool<AstNode> *nodes;

  Vector<Source> *sources;
};

b32 type_check_module(TypeCheckContext ctx, AstNode *module);

b32 generate_c_code(AstNode *mod);

// -[ Message ]-

enum MessageSeverity : u8 {
  Error,
  Warning,
  Info,
};

union MessageArg {
  TokenKind token_kind;
};

struct Message {
  MessageSeverity severity;

  SourceIdx src_idx = UINT32_MAX;
  SourceSpan span;

  Str format;
  MessageArg args[0];
};

struct MessageManager {
  Arena arena;
  Vector<Message *> messages;

  void init(Allocator list_alloc) {
    arena.init(MiB(16));
    messages.init(list_alloc);
  }

  void deinit();

  void print_messages(Slice<Source> sources);
  void error(SourceIdx src_idx, SourceSpan span, char const *format, ...);
};

// -[ Compiler context ]-

enum JobKind : u8 {
  Job_read_file,
  Job_tokenize,
  Job_parse,
};

struct Job {
  JobKind kind;
  SourceIdx src_idx;
};

struct Compiler;

typedef void (*JobCompletionListenerFn)(Compiler *compiler, JobKind kind, Source *source);

struct Compiler {
  Arena arena;
  Arena work_arena;

  MessageManager messages;

  StringInterner strings;
  TypeInterner types;
  ObjectPool<AstNode> nodes;
  EnvManager envs;

  Vector<Source> sources;
  Queue<Job> jobs;

  JobCompletionListenerFn listener = nullptr;

  // ---

  void init();
  void compile_file(Str filename);

  void register_job_completion_listener(JobCompletionListenerFn f) { listener = f; }

  SourceIdx add_read_file_job(Str filename);
};

// clang-format off
//#define Push_message(Messages, Msg) { Message _tmp = Msg; _tmp.file = __FILE__; _tmp.line = __LINE__; (Messages).push(_tmp); }
// clang-format on
