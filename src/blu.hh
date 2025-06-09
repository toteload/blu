#pragma once

#include "toteload.hh"
#include "vector.hh"
#include "hashmap.hh"

#include <stdio.h>

// -[ Source location ]-

struct SourceLocation {
  u32 line      = 0;
  u32 col       = 0;
  char const *p = nullptr;
};

struct SourceSpan {
  SourceLocation start;
  SourceLocation end;

  usize len() { return end.p - start.p; }
  Str str() { return {start.p, cast<u32>(len())}; }
};

// -[ String interner ]-

struct StrKey {
  u32 idx;
};

#define XXH_INLINE_ALL
#include "xxhash.h"

ttld_inline u32 str_hash(Str s) { return XXH32(s.str, s.len, 0); }

struct StringInterner {
  Allocator storage;
  HashMap<Str, StrKey, str_eq, str_hash> map;
  Vector<Str> list;

  void init(Allocator storage_allocator, Allocator map_allocator, Allocator list_allocator);
  void deinit();

  StrKey add(Str s);
  Str get(StrKey key) { return list[key.idx]; }
};

// -[ Token ]-

enum TokenKind : u32 {
  Tok_colon,
  Tok_arrow,
  Tok_semicolon,
  Tok_equals,
  Tok_minus,
  Tok_plus,

  Tok_less_than,
  Tok_less_equal_than,

  Tok_comma,

  Tok_literal_int,

  Tok_brace_open,
  Tok_brace_close,
  Tok_paren_open,
  Tok_paren_close,

  Tok_keyword_fn,
  Tok_keyword_return,
  Tok_keyword_if,
  Tok_keyword_else,

  Tok_identifier,

  Tok_kind_max,
};

struct Token {
  TokenKind kind;
  SourceSpan span;

  Str str() { return Str{span.start.p, cast<u32>(span.len())}; }
};

enum TokenizerResult : u32 {
  TokResult_ok,
  TokResult_end,
  TokResult_unrecognized_token,
};

b32 tokenize(char const *source, usize len, Vector<Token> *tokens);

// -[ AST ]-

enum BinaryOpKind : u32 {
  Sub,
  Add,

  LessEqual,

  BinaryOpKind_max,
};

enum UnaryOpKind : u32 {
  Negate,

  UnaryOpKind_max,
};

enum AstKind : u32 {
  Ast_module,

  Ast_param,
  Ast_function,

  Ast_scope,

  Ast_identifier,
  Ast_literal_int,
  Ast_declaration,
  Ast_assign,

  Ast_call,
  Ast_if_else,
  Ast_binary_op,
  Ast_unary_op,

  Ast_return,

  Ast_kind_max,
};

using AstRef = u32;

constexpr AstRef nil = UINT32_MAX;

struct AstNode {
  AstKind kind;
  SourceSpan span;

  union {
    struct {
      Vector<AstRef> params;
      AstRef name;
      AstRef return_type;
      AstRef body;
    } function;

    struct {
      AstRef name;
      AstRef type;
    } param;

    struct {
      AstRef name;
      AstRef type;
      AstRef initial_value;
    } declaration;

    struct {
      AstRef lhs;
      AstRef value;
    } assign;

    struct {
      AstRef f;
      Vector<AstRef> arguments;
    } call;

    struct {
      AstRef cond;
      AstRef then;
      AstRef otherwise;
    } if_else;

    struct {
      BinaryOpKind kind;
      AstRef lhs;
      AstRef rhs;
    } binary_op;

    struct {
      UnaryOpKind kind;
      AstRef value;
    } unary_op;

    struct {
      AstRef value;
    } ret;

    struct {
      StrKey key;
    } identifier;

    struct {
      Vector<AstRef> statements;
    } scope;

    struct {
      Vector<AstRef> items;
    } module;
  };
};

struct ParseContext {
  StringInterner *strings;
  Allocator ref_allocator;
  Vector<AstNode> *nodes;
};

b32 parse(ParseContext *ctx, Slice<Token> tokens, AstRef *root);

b32 generate_c_code(FILE *out, Slice<AstNode> nodes, AstRef mod);
