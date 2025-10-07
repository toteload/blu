#pragma once

#include "toteload.hh"
#include "vector.hh"
#include "queue.hh"
#include "hashmap.hh"
#include "segment_list.hh"
#include "index.hh"

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

struct ParseContext {
  MessageManager *messages;
  Tokens *tokens;
};

b32 parse(ParseContext *ctx, AstNodes *nodes);

ttld_inline b32 eq_node_index(void *context, NodeIndex a, NodeIndex b) { return a == b; }
ttld_inline u32 hash_node_index(void *context, NodeIndex a) { return a.get(); }

using TypeAnnotations = HashMap<NodeIndex, TypeIndex, eq_node_index, hash_node_index>;

#include "value.hh"
#include "env.hh"

struct Source {
  Str filename;
  Str source;
  Tokens *tokens;
  AstNodes *nodes;

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
  ValueStore *values;
  TypeAnnotations *type_annotations;
};

b32 type_check(TypeCheckContext *ctx, Source *source);

#include "hir.hh"

struct CodeGeneratorContext {
  MessageManager *messages;
  Arena *output_arena;
  ValueStore *values;
  TypeInterner *types;
  TypeAnnotations *type_annotations;
};

b32 generate_c_code(CodeGeneratorContext *ctx, Source *source);

// -[ Message ]-

enum MessageSeverity : u8 {
  Error,
  Warning,
  Info,
};

union MessageArg {
  TokenKind token_kind;
  StrKey strkey;
  TypeIndex type;
};

struct Message {
  MessageSeverity severity;

  Str format;
  MessageArg args[0];
};

struct MessageManager {
  Arena arena;
  Vector<Message *> messages;
  StringInterner *strings;
  TypeInterner *types;

  void init(Allocator list_alloc, StringInterner *strings, TypeInterner *types) {
    arena.init(MiB(16));
    messages.init(list_alloc);
    this->strings = strings;
    this->types   = types;
  }

  void deinit();

  void print_message(Message *message);

  void print_messages();
  void error(char const *format, ...);
};

void debug_print_type(TypeInterner *types, TypeIndex type);
