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

  char const *file = nullptr;
  u32 line         = 0;
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
  Tok_plus_equals,
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
  Tok_dot,
  Tok_literal_int,
  Tok_brace_open,
  Tok_brace_close,
  Tok_paren_open,
  Tok_paren_close,
  Tok_bracket_open,
  Tok_bracket_close,
  Tok_keyword_fn,
  Tok_keyword_return,
  Tok_keyword_if,
  Tok_keyword_else,
  Tok_keyword_while,
  Tok_keyword_break,
  Tok_keyword_continue,
  Tok_keyword_and,
  Tok_keyword_or,
  Tok_keyword_for,
  Tok_keyword_in,
  Tok_identifier,
  Tok_builtin_run,
  Tok_line_comment,
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

  Assign,

  AddAssign,

  BinaryOpKind_max,
};

enum UnaryOpKind : u32 {
  Negate,
  Not,
  AddressOf,

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
  Ast_for,

  Ast_call,
  Ast_if_else,
  Ast_binary_op,
  Ast_unary_op,

  Ast_cast,

  Ast_type_pointer,
  Ast_type_slice,
  Ast_deref,

  Ast_return,

  Ast_kind_max,
};

struct AstNode {
  AstKind kind;
  SourceSpan span;
  Type *type    = nullptr;
  AstNode *next = nullptr;

  union {
    struct {
      AstNode *params;
      AstNode *return_type;
      AstNode *body;
    } function;

    struct {
      AstNode *name;
      AstNode *type;
    } param;

    struct {
      b32 is_const;
      AstNode *name;
      AstNode *type;
      AstNode *value;
    } declaration;

    struct {
      AstNode *lhs;
      AstNode *value;
    } assign;

    struct {
      AstNode *cond;
      AstNode *then;
      AstNode *otherwise;
    } if_else;

    struct {
      AstNode *callee;
      AstNode *args;
    } call;

    struct {
      BinaryOpKind kind;
      AstNode *lhs;
      AstNode *rhs;
    } binary_op;

    struct {
      UnaryOpKind kind;
      AstNode *value;
    } unary_op;

    struct {
      AstNode *destination_type;
      AstNode *value;
    } cast;

    struct {
      AstNode *value;
    } _return;

    struct {
      StrKey key;
    } identifier;

    struct {
      AstNode *cond;
      AstNode *body;
    } _while;

    struct {
      AstNode *item;
      AstNode *iterable;
      AstNode *body;
    } _for;

    struct {
      AstNode *expressions;
    } scope;

    struct {
      AstNode *items;
    } module;

    struct {
      AstNode *base;
    } pointer;

    struct {
      AstNode *base;
    } slice;

    struct {
      AstNode *value;
    } deref;
  };
};

#define ForEachAstNode(i, n) for (AstNode *i = n; i; i = i->next)

b32 parse(CompilerContext *ctx);

void debug_ast_node(AstNode *n);

// -[ Types ]-

enum TypeKind : u16 {
  Type_IntegerConstant,
  Type_Integer,
  Type_Boolean,
  Type_Function,
  Type_Void,
  Type_Never,
  Type_Pointer,
  Type_slice,
};

enum Signedness : u8 {
  Signed,
  Unsigned,
};

struct Type {
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
      Type *base_type;
    } slice;
    struct {
      u32 param_count;
      Type *return_type;
      Type *params[0];
    } function;
  };

  b32 is_sized_type() { return kind != Type_Void && kind != Type_Never; }

  u32 byte_size() {
    switch (kind) {
    case Type_Integer:
    case Type_IntegerConstant:
    case Type_Void:
    case Type_Never:
    case Type_Pointer:
    case Type_slice:
    case Type_Boolean:
      return sizeof(*this);
    case Type_Function: {
      return sizeof(*this) + function.param_count * sizeof(Type *);
    } break;
    }
  }

  bool is_integer_or_integer_constant() {
    return kind == Type_Integer || kind == Type_IntegerConstant;
  }

  static Type make_slice(Type *base) {
    Type t;
    t.kind            = Type_slice;
    t.slice.base_type = base;
    return t;
  }
  static Type make_void() { return {Type_Void, {}}; }
  static Type make_bool() { return {Type_Boolean, {}}; }
  static Type make_never() { return {Type_Never, {}}; }
  static Type make_integer_constant() { return {Type_IntegerConstant, {}}; }
  static Type make_integer(Signedness s, u16 width) {
    return {
      Type_Integer,
      {{
        s,
        width,
      }},
    };
  }
};

b32 type_eq(void *context, Type *a, Type *b);
u32 type_hash(void *context, Type *x);

struct TypeInterner {
  Allocator storage;
  HashMap<Type *, Type *, type_eq, type_hash> map;

  // Often used and always available
  Type *_bool;
  Type *_i32;
  Type *_integer_constant;
  Type *_void;
  Type *_never;

  void init(Arena *arena, Allocator storage_allocator, Allocator map_allocator);
  void deinit();

  Type *add(Type *type);
};

b32 infer_types(CompilerContext *ctx, AstNode *root);

// -[ C code generation ]-

b32 generate_c_code(CompilerContext *ctx, FILE *out, AstNode *mod);

// -[ ]-

enum ValueKind : u8 {
  Value_type,

  Value_param,
  Value_local,

  Value_iter_item,

  Value_builtin,
};

struct Value {
  ValueKind kind;

  union {
    Type *type;

    struct {
      Type *type;
      AstNode *ast;
    } param;

    struct {
      Type *type;
      AstNode *ast;
    } local;

    struct {
      Type *type;
    } builtin;

    struct {
      Type *type;
      AstNode *ast;
    } iter_item;
  };

  static Value make_type(Type *type) {
    Value val;
    val.kind = Value_type;
    val.type = type;
    return val;
  }

  static Value make_param(AstNode *ast, Type *type) {
    Value val;
    val.kind       = Value_param;
    val.param.ast  = ast;
    val.param.type = type;
    return val;
  }

  static Value make_local(AstNode *ast, Type *type) {
    Value val;
    val.kind       = Value_local;
    val.param.ast  = ast;
    val.param.type = type;
    return val;
  }

  static Value make_builtin(Type *type) {
    Value val;
    val.kind         = Value_builtin;
    val.builtin.type = type;
    return val;
  }

  static Value make_iter_item(Type *type, AstNode *n) {
    Value val;
    val.kind = Value_iter_item;
    val.iter_item.type = type;
    val.iter_item.ast = n;
    return val;
  }
};

// This makes sure that no matter variant Value is, you can always access the type of the value
// through the type field.
// This may not be a good idea :)
static_assert(offsetof(Value, type) == offsetof(Value, param.type));
static_assert(offsetof(Value, type) == offsetof(Value, local.type));

// -[ Environment ]-

ttld_inline b32 str_key_eq(void *context, StrKey a, StrKey b) { return a.idx == b.idx; }
ttld_inline u32 str_key_hash(void *context, StrKey x) { return x.idx; }

struct Env {
  Env *parent;
  HashMap<StrKey, Value, str_key_eq, str_key_hash> map;

  void init(Allocator allocator, Env *parent = nullptr) {
    map.init(allocator);
    this->parent = parent;
  }

  void deinit() {
    parent = nullptr;
    map.deinit();
  }

  void insert(StrKey identifier, Value val) { map.insert(identifier, val); }

  Value *lookup(StrKey identifier) {
    Value *p = map.get_ptr(identifier);
    if (p) {
      return p;
    }

    if (parent) {
      return parent->lookup(identifier);
    }

    return nullptr;
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
    env->init(env_allocator, parent);
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
  AstNode *root;
};

// clang-format off
#define Push_message(Messages, Msg) { Message _tmp = Msg; _tmp.file = __FILE__; _tmp.line = __LINE__; (Messages).push(_tmp); }
// clang-format on
