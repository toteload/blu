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

struct AstNodeTag {};
using NodeIndex         = Index<u32, AstNodeTag>;
using OptionalNodeIndex = OptionalIndex<u32, AstNodeTag>;

struct TypeIndexTag {};
using TypeIndex         = Index<u32, TypeIndexTag>;
using OptionalTypeIndex = OptionalIndex<u32, TypeIndexTag>;

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

struct Messages;

#include "types.hh"

template<typename T> struct Span {
  T start;
  T end;
};

#include "tokens.hh"

b32 tokenize(Messages *messages, Str source, Tokens *output);
void write_tokens(Tokens *tokens, Str source, Arena *out);

#include "ast.hh"

struct ParseContext {
  Messages *messages;
  Tokens *tokens;
};

b32 parse(ParseContext *ctx, Str source, AstNodes *nodes);

ttld_inline b32 eq_node_index(void *context, NodeIndex a, NodeIndex b) { return a == b; }
ttld_inline u32 hash_node_index(void *context, NodeIndex a) { return a.inner(); }

#include "value.hh"
#include "env.hh"

struct Source {
  Str filename;
  Str source;
  Tokens *tokens  = nullptr;
  AstNodes *nodes = nullptr;

  Str get_token_str(TokenIndex idx) {
    auto span = tokens->span(idx);
    return source.sub(span.start, span.end);
  }

  NodeIndex root_node_index() {
    NodeIndex idx = {0};
    Debug_assert(nodes->kind(idx) == Ast_root);
    return idx;
  }
};

enum DeclarationKind {
  Declaration_of_type,
  Declaration_of_value,
  Declaration_of_undetermined,
};

struct Declaration {
  DeclarationKind kind;
  union {
    TypeIndex type;
    NodeIndex node_index;
  };
};

struct TypeCheckContext {
  Messages *messages;
  EnvManager<Declaration> *envs;
  TypeInterner *types;
  StringInterner *strings;
  ValueStore *values;
  Arena *work_arena;
};

b32 typecheck(TypeCheckContext *context, Source *source, Slice<TypeIndex> annotations);

#include "interpreter.hh"

// -[ Message ]-

enum MessageLocationKind : u32 {
  MessageLocation_none,
  MessageLocation_token_index,
  MessageLocation_node_index,
};

struct MessageLocation {
  MessageLocationKind kind;
  union {
    TokenIndex token_index;
    NodeIndex node_index;
  } data;
};

enum MessageSeverity : u8 {
  Error,
  Warning,
  Info,
};

union MessageArg {
  TokenKind token_kind;
  StrKey strkey;
  TypeIndex type;
  Span<u32> span;
};

struct Message {
  MessageLocation location;
  MessageSeverity severity;
  u32 format_len;     // excluding null terminator
  char const *format; // null terminated
  MessageArg *args;
};

struct Messages {
  Arena arena;
  Vector<Message *> messages;
  StringInterner *strings;
  TypeInterner *types;
  Source *source = nullptr;

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
  void error(TokenIndex location, char const *format, ...);
  void error(NodeIndex location, char const *format, ...);

  void _error(MessageLocation location, char const *format, va_list varargs);
};

void debug_print_type(TypeInterner *types, TypeIndex type);
