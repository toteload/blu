#ifndef AST_H
#define AST_H

#include "blu.h"
#include "tokens.h"

typedef enum {
  Ast_source,
  Ast_mod_section,
  Ast_block,
  Ast_type_slice,
  Ast_type_array,
  Ast_type_function,
  Ast_builtin,
  Ast_declaration,
  Ast_assign,
  Ast_literal_int, // TokenOffset
  Ast_literal_string, // TokenOffset
  Ast_literal_u8_string, // TokenOffset
  Ast_identifier, // TokenOffset
  Ast_label, // TokenOffset
  Ast_call,
  Ast_index,
  Ast_unary_op,
  Ast_binary_op,
  Ast_function,
  Ast_param,
  Ast_if_else,
  Ast_while,
  Ast_break,
  Ast_cast,
  Ast_as,

  // Make sure to update Ast_kind_max when you update this enum!
} AstKind;

#define Ast_kind_max (Ast_as + 1)

typedef enum {
  Builtin_debug,
  Builtin_len,
} BuiltinKind;

typedef enum {
#define WHEN_ASSIGN(...)
#define WHEN_OP(...) __VA_ARGS__
#define X(name, kind, _0, _1, _2) WHEN_##kind(name,)
#include "x_binops.h"
#undef X
#undef WHEN_OP
#undef WHEN_ASSIGN
} BinaryOpKind;

typedef enum {
#define WHEN_ASSIGN(...) __VA_ARGS__
#define WHEN_OP(...)
#define X(name, kind, _0, _1, _2) WHEN_##kind(name,)
#include "x_binops.h"
#undef X
#undef WHEN_OP
#undef WHEN_ASSIGN
} AssignKind;

typedef enum {
  Negate,
  Logical_not,
  Bit_not,
} UnaryOpKind;

typedef struct {
  u8 kind;
  u32 count;
  AstKey args[];
} AstBuiltin;

typedef struct {
  AstKey return_type;
  u32 count;
  AstKey param_types[];
} AstTypeFunction;

typedef struct {
  AstKey base;
} AstTypeSlice;

typedef struct {
  AstKey size;
  AstKey base;
} AstTypeArray;

typedef struct {
  TokenOffset name;
  AstKey  type;
  AstKey  value;
} AstDeclaration;

typedef struct {
  u32 count;
  AstKey items[];
} AstSource;

typedef struct {
  TokenOffset name;
  u32 count;
  AstKey items[];
} AstModSection;

typedef struct {
  AstKey label;
  u32 count;
  AstKey items[];
} AstBlock;

typedef struct {
  TokenOffset name;
  AstKey    type;
} AstParam;

typedef struct {
  AstKey return_type;
  AstKey body;
  u32      count;
  AstKey params[];
} AstFunction;

typedef struct {
  AstKey cond;
  AstKey then;
  AstKey otherwise;
} AstIfElse;

typedef struct {
  AstKey label;
  AstKey cond;
  AstKey body;
} AstWhile;

typedef struct {
  u8 op_kind;
  AstKey value;
} AstUnaryOp;

typedef struct {
  u8 op_kind;
  AstKey    lhs;
  AstKey    rhs;
} AstBinaryOp;

typedef struct {
  AstKey callee;
  u32 count;
  AstKey args[];
} AstCall;

typedef struct {
  AstKey indexable;
  AstKey index_at;
} AstIndexData;

typedef struct {
  u8 kind;
  AstKey lhs;
  AstKey value;
} AstAssign;

typedef struct {
  AstKey label;
  AstKey value;
} AstBreak;

typedef struct {
  AstKey type;
  AstKey value;
} AstCast;

typedef union {
  AstSource          source;
  AstModSection      mod_section;
  AstBlock           block;
  AstBuiltin         builtin;
  AstTypeFunction    type_function;
  AstTypeSlice       type_slice;
  AstTypeArray       type_array;
  AstDeclaration     declaration;
  AstAssign          assign;
  TokenOffset        token_index;
  AstCall            call;
  AstIndexData       index;
  AstUnaryOp         unary_op;
  AstBinaryOp        binary_op;
  AstFunction        function;
  AstParam           param;
  AstIfElse          if_else;
} AstNodeData;

typedef struct {
  TokenOffset start;
  TokenOffset end;
} SpanToken;

typedef struct {
  MessageSink *msg_sink;
  Arena *arena;
  Arena *scratch;
} ParseContext;

typedef struct {
  u32        count;
  u8        *kinds;
  SpanToken *spans;
  u32       *datas;
  void      *extra;
} AstNodes;

void *ast_data(AstNodes *ast, AstKey idx);
AstKind ast_kind(AstNodes *ast, AstKey idx);
SpanToken ast_span(AstNodes *ast, AstKey idx);

String ast_kind_string(AstKind kind);

b32 parse(ParseContext *context, Tokens *tokens, AstNodes *ast);

#endif // AST_H
