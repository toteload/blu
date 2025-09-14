#pragma once

#include "toteload.hh"
#include "vector.hh"
#include "queue.hh"
#include "hashmap.hh"
#include "segment_list.hh"

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

template<typename T> struct Span {
  T start;
  T end;
};

#include "tokens.hh"

b32 tokenize(MessageManager *messages, Str source, Tokens *output);

#include "ast.hh"

// ttld_inline SourceSpan get_ast_source_span(AstNode *node, Slice<Token> tokens) {
//   SourceLocation start = tokens[node->token_span.start].span.start;
//   SourceLocation end   = tokens[node->token_span.end].span.end;
//   return {
//     start,
//     end,
//   };
// }

struct ParseContext {
  MessageManager *messages;
  Tokens *tokens;
};

b32 parse(ParseContext *ctx, Nodes *nodes);

#include "value.hh"
#include "env.hh"

struct Source {
  Str filename;
  Str source;
  Tokens *tokens;
  Nodes *nodes;

  Str get_token_str(TokenIndex idx) {
    auto span = tokens->span(idx);
    return source.sub(span.start, span.end);
  }
};

struct TypeCheckContext {
  MessageManager *messages;
  Arena *work_arena;
  EnvManager *envs;
  TypeInterner *types;
  StringInterner *strings;
};

b32 type_check(TypeCheckContext *ctx, Source *source);

// -[ Message ]-

enum MessageSeverity : u8 {
  Error,
  Warning,
  Info,
};

union MessageArg {
  TokenKind token_kind;
  Type *type;
};

struct Message {
  MessageSeverity severity;

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

  void print_messages();
  void error(char const *format, ...);
};

// -[ Compiler context ]-

// enum JobKind : u8 {
//   Job_read_file,
//   Job_tokenize,
//   Job_parse,
// };
//
// struct Job {
//   JobKind kind;
//   SourceIdx src_idx;
// };
//
// struct Compiler;
//
// typedef void (*JobCompletionListenerFn)(Compiler *compiler, JobKind kind, Source *source);
//
// struct Compiler {
//   Arena arena;
//   Arena work_arena;
//
//   MessageManager messages;
//
//   StringInterner strings;
//   TypeInterner types;
//   EnvManager envs;
//
//   Vector<Source> sources;
//   Queue<Job> jobs;
//
//   JobCompletionListenerFn listener = nullptr;
//
//   // ---
//
//   void init();
//   void compile_file(Str filename);
//
//   void register_job_completion_listener(JobCompletionListenerFn f) { listener = f; }
//
//   SourceIdx add_read_file_job(Str filename);
// };
