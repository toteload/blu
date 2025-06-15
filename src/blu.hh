#pragma once

#include "toteload.hh"
#include "vector.hh"
#include "hashmap.hh"

#include <stdio.h>

struct CompilerContext;
struct Type;

// -[ Source location ]-

struct SourceLocation {
  u32 line      = 0;
  u32 col       = 0;
  char const *p = nullptr;
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
        loc.p + 1,
      }
    };
  }

  usize len() { return end.p - start.p; }
  Str str() { return {start.p, cast<u32>(len())}; }
};

// -[ String interner ]-

struct StrKey {
  u32 idx;
};

ttld_inline b32 str_eq_with_context(void *context, Str a, Str b) { return str_eq(a, b); }
u32 str_hash(void *context, Str s);

struct StringInterner {
  Allocator storage;
  HashMap<Str, StrKey, str_eq_with_context, str_hash> map;
  Vector<Str> list;

  void init(Allocator storage_allocator, Allocator map_allocator, Allocator list_allocator);
  void deinit();

  StrKey add(Str s);
  Str get(StrKey key) { return list[key.idx]; }
};

// -[ Compiler message ]-

enum MessageSeverity : u32 {
  Error,
  Warning,
  Info,
};

struct Message {
  SourceSpan span;
  MessageSeverity severity;
  Str message;
};

// -[ Token ]-

enum TokenKind : u32 {
  Tok_colon,
  Tok_arrow,
  Tok_semicolon,
  Tok_equals,

  Tok_minus,
  Tok_plus,
  Tok_star,
  Tok_slash,
  Tok_percent,

  Tok_exclamation,
  Tok_ampersand,
  Tok_bar,
  Tok_caret,
  Tok_tilde,
  Tok_left_shift,
  Tok_right_shift,

  Tok_cmp_eq,
  Tok_cmp_ne,
  Tok_cmp_gt,
  Tok_cmp_ge,
  Tok_cmp_lt,
  Tok_cmp_le,

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
  Tok_keyword_while,
  Tok_keyword_break,
  Tok_keyword_continue,

  Tok_keyword_and,
  Tok_keyword_or,

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

b32 tokenize(CompilerContext *ctx, char const *source, usize len);

// -[ AST ]-

enum BinaryOpKind : u32 {
  Mul,
  Div,
  Mod,

  Sub,
  Add,

  Bit_shift_left,
  Bit_shift_right,

  Bit_and,
  Bit_or,
  Bit_xor,

  Cmp_equal,
  Cmp_not_equal,
  Cmp_greater_than,
  Cmp_greater_equal,
  Cmp_less_than,
  Cmp_less_equal,

  Logical_and,
  Logical_or,

  BinaryOpKind_max,
};

enum UnaryOpKind : u32 {
  Negate,
  Not,

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

  Ast_while,
  Ast_break,
  Ast_continue,

  Ast_call,
  Ast_if_else,
  Ast_binary_op,
  Ast_unary_op,

  Ast_return,

  Ast_kind_max,
};

struct AstNode;

using AstRef = AstNode *;

constexpr AstRef nil = nullptr;

struct AstNode {
  AstKind kind;
  SourceSpan span;
  Type *type = nullptr;
  AstNode *next = nullptr;

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
      AstRef cond;
      AstRef body;
    } _while;

    struct {
      Vector<AstRef> statements;
    } scope;

    struct {
      Vector<AstRef> items;
    } module;
  };
};

b32 parse(CompilerContext *ctx);

// -[ Types ]-

enum TypeKind : u16 {
  Integer,
  Boolean,
  Function,
};

enum Signedness : u8 {
  Signed,
  Unsigned,
};

struct Type {
  Type *next;
  TypeKind kind;

  union {
    struct {
      Signedness signedness;
      u16 bitwidth;
    } integer;
    struct {
      Type *base_type;
    } pointer;
    struct {
      Type *return_type;
      Type *params;
    } function;
  };

  static Type make_bool() { return {nullptr, Boolean}; }
  static Type make_integer(Signedness s, u16 width) {
    return {
      nullptr,
      Integer,
      {{
        s,
        width,
      }},
    };
  }

  static Type make_function(Type *return_type, Type *params) {
    Type t;
    t.kind = Function;
    t.function.return_type = return_type;
    t.function.params = params;
    return t;
  }
};

b32 type_eq(void *context, Type a, Type b);
u32 type_hash(void *context, Type x);

struct TypeInterner {
  HashMap<Type, Type *, type_eq, type_hash> map;
  ObjectPool<Type> pool;

  void init(Allocator map_allocator, Allocator pool_allocator);
  void deinit();

  Type *add(Type type);
};

b32 infer_types(CompilerContext *ctx, AstRef root);

// -[ C code generation ]-

b32 generate_c_code(FILE *out, AstRef mod);

// -[ ]-

enum ValueKind : u8 {
  Value_type,
  Value_ast,
};

struct Value {
  ValueKind kind;

  union {
    Type *type;
    AstNode *ast;
  };

  static Value make_type(Type *type) {
    return {
      Value_type,
      {
        type,
      },
    };
  }
};

// -[ Environment ]-

ttld_inline b32 str_key_eq(void *context, StrKey a, StrKey b) { return a.idx == b.idx; }
ttld_inline u32 str_key_hash(void *context, StrKey x) { return x.idx; }

struct Env {
  Env *parent;
  HashMap<StrKey, Value, str_key_eq, str_key_hash> map;

  void init(Allocator allocator) { map.init(allocator); }

  void deinit();

  void insert(StrKey identifier, Value val) { map.insert(identifier, val); }

  Value *lookup(StrKey identifier) {
    Value *p = map.get_ptr(identifier);
    if (p) {
      return p;
    }

    if (!parent) {
      return nullptr;
    }

    return parent->lookup(identifier);
  }
};

struct EnvManager {
  ObjectPool<Env> pool;
  Allocator env_allocator;

  void init(Allocator pool_allocator, Allocator env_allocator) {
    this->env_allocator = env_allocator;

    pool.init(pool_allocator);
  }

  void deinit();

  Env *alloc(Env *parent = nullptr) {
    Env *env = pool.alloc();
    env->init(env_allocator);
    return env;
  }

  void dealloc(Env *env) {
    env->deinit();
    pool.dealloc(env);
  }
};

// -[ Compiler context ]-

struct CompilerContext {
  Arena arena;
  Vector<Message> messages;

  Allocator ref_allocator;

  StringInterner strings;
  TypeInterner types;

  Vector<Token> tokens;
  ObjectPool<AstNode> nodes;

  EnvManager environments;

  Env *global_environment;
  AstRef root;
};
