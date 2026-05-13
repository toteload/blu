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

struct ValueIndexTag {};
using ValueIndex         = Index<u32, ValueIndexTag>;
using OptionalValueIndex = OptionalIndex<u32, ValueIndexTag>;

enum NodeIndexKind : u8 {
  NodeIndex_ast_node,
  NodeIndex_value,
};

struct NodeIndex {
  NodeIndexKind kind;

  // A value of 0 means not valid or nil.
  u32 idx;

  bool is_some() const { return idx != 0; }
  bool is_none() const { return idx == 0; }

  static NodeIndex none() {
    return {
      .kind = NodeIndex_ast_node, // The kind is invalid if `idx == 0`.
      .idx  = 0,
    };
  }
};

struct TypeIndexTag {};
using TypeIndex = Index<u32, TypeIndexTag>;

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

struct TokenizeContext {
  Messages *messages;
};

b32 tokenize(TokenizeContext *context, Str source, Tokens *out);

void write_tokens(Tokens *tokens, Str source, Arena *out);

#include "ast.hh"

struct ParseContext {
  Messages *messages;
};

b32 parse_root(ParseContext *ctx, Tokens *tokens, AstNodes *out);

ttld_inline b32 eq_node_index(void *context, NodeIndex a, NodeIndex b) {
  return a.kind == b.kind && a.idx == b.idx;
}
ttld_inline u32 hash_node_index(void *context, NodeIndex a) {
  return (cast<u32>(a.kind) << 30) | a.idx;
}

#include "value.hh"
#include "env.hh"

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
    NodeIndex  node_index;
  } data;
};

enum MessageSeverity : u8 {
  Error,
  Warning,
  Info,
};

union MessageArg {
  TokenKind token_kind;
  StrKey    strkey;
  TypeIndex type;
  Span<u32> span;
};

struct Message {
  MessageLocation location;
  MessageSeverity severity;
  u32             format_len; // excluding null terminator
  char const     *format;     // null terminated
  MessageArg     *args;
};

struct MessageContext {
  Str             text;
  Tokens         *tokens;
  AstNodes       *nodes;
  TypeInterner   *types;
  StringInterner *strings;
};

struct Messages {
  Arena             arena;
  Vector<Message *> messages;

  void init(Allocator vector_alloc) {
    arena.init(MiB(16));
    messages.init(vector_alloc);
  }

  void deinit() {
    messages.deinit();
    arena.deinit();
  }

  void print_message(MessageContext *context, Message *message);
  void print_messages(MessageContext *context);

  void error(char const *format, ...);
  void error(TokenIndex location, char const *format, ...);
  void error(NodeIndex location, char const *format, ...);

  void _error(MessageLocation location, char const *format, va_list varargs);
};

// ---

struct ParsedSource {
  Str       text;
  Tokens   *tokens;
  AstNodes *nodes;
};

ttld_inline Str get_token_str(Str text, Tokens *tokens, TokenIndex idx) {
  auto span = tokens->span(idx);
  return text.sub(span.start, span.end);
}

#include "interpreter.hh"

enum SourceUnitStage : u8 {
  Stage_init = 0,
  Stage_tokenize,
  Stage_parse,
  Stage_typecheck,
  Stage_run_const_code,
  Stage_done,
};

struct SourceUnit {
  SourceUnitStage stage;

  Str filename;
  Str text;

  Arena arena;
  Arena work_arena;

  Messages messages;

  StringInterner strings;
  TypeInterner   types;

  Interpreter interpreter;

  Tokens            tokens;
  AstNodes          nodes;
  Vector<TypeIndex> node_types;

  void init(Str filename, Str text);
  void deinit();

  bool tokenize();
  bool parse();
  bool typecheck();
  bool run_const_code();
  bool run_main(ValueIndex *result);

  void print_messages();
};

enum DeclarationKind {
  Declaration_of_type,
  Declaration_of_value,

  Declaration_unresolved,
  Declaration_resolving,
};

struct Declaration {
  DeclarationKind kind;
  union {
    TypeIndex type;
    NodeIndex node_index;
  };
};

struct TypeCheckContext {
  Messages                *messages;
  EnvManager<Declaration> *envs;
  TypeInterner            *types;
  StringInterner          *strings;
  Arena                   *work_arena;
};

b32 typecheck(
  TypeCheckContext *context, ParsedSource *source, Slice<TypeIndex> node_types, NodeIndex idx
);

void debug_print_type(TypeInterner *types, TypeIndex type);
u32  string_literal_byte_size(Str literal);
u32  decode_string_literal(Str literal, char *out);
void ast_pretty_print(Str text, Tokens *tokens, TypeInterner *types, ValueStore *values, AstNodes *nodes, NodeIndex idx);
