#ifndef AST_H
#define AST_H

#include "blu.h"

enum AstKind {
  Ast_root,
  Ast_block,

  Ast_type_slice,
  Ast_type_array,
  Ast_type_function,

  Ast_builtin,

  Ast_declaration,
  Ast_assign,
  Ast_literal_sequence,
  Ast_literal_int,
  Ast_literal_string,
  Ast_identifier,
  Ast_call,
  Ast_index,
  Ast_unary_op,
  Ast_binary_op,
  Ast_function,
  Ast_param,
  Ast_if_else,
  Ast_for,
  Ast_defer,
  Ast_const,
  Ast_cast,

  Ast_kind_max,
};

enum BuiltinKind {
  Builtin_print,
};

enum BinaryOpKind {
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

enum AssignKind {
  Assign_normal,

  AssignKind_max,
};

enum UnaryOpKind {
  Negate,
  Not,

  UnaryOpKind_max,
};

typedef u32 NodeIndex;

#define SEGMENTLIST_NAME          NodeIndexList
#define SEGMENTLIST_TYPE          NodeIndex
#define SEGMENTLIST_MIN_SIZE_LOG2 3
#define SEGMENTLIST_SEGMENT_COUNT 24
#include "segment_list.h"

#define Max_nodes_in_NodeIndexList ((usize)1 << (3 + 24))

struct {
  u8 kind;
  NodeIndexList args;
} AstBuiltin;

struct {
  NodeIndex     return_type;
  NodeIndexList param_types;
} AstTypeFunction;

struct {
  NodeIndex base;
} AstTypeSlice;

struct {
  NodeIndex size;
  NodeIndex base;
} AstTypeArray;

struct {
  TokenIndex name;
  NodeIndex  type;
  NodeIndex  value;
} AstDeclaration;

struct {
  NodeIndexList items;
} AstRoot;

struct {
  NodeIndexList items;
} AstBlock;

struct {
  NodeIndexList items;
} AstLiteralSequence;

struct {
  TokenIndex name;
  NodeIndex  type;
} AstParam;

struct {
  NodeIndexList params;
  NodeIndex     return_type;
  NodeIndex     body;
} AstFunction;

struct {
  NodeIndex cond;
  NodeIndex then;
  NodeIndex otherwise;
} AstIfElse;

struct {
  NodeIndex iterable;
  NodeIndex iterator;
  NodeIndex body;
} AstFor;

struct {
  UnaryOpKind kind;
  NodeIndex   value;
} AstUnaryOp;

struct {
  BinaryOpKind kind;
  NodeIndex    lhs;
  NodeIndex    rhs;
} AstBinaryOp;

struct {
  NodeIndex base;
  NodeIndex field;
} AstFieldAccess;

struct {
  NodeIndex     callee;
  NodeIndexList args;
} AstCall;

struct {
  NodeIndex indexable;
  NodeIndex index_at;
} AstIndexData;

struct {
  NodeIndex value;
} AstDefer;

struct {
  AssignKind kind;
  NodeIndex  lhs;
  NodeIndex  value;
} AstAssign;

struct {
  NodeIndex expr;
} AstConst;

struct {
  NodeIndex type_dst;
  NodeIndex value;
} AstCast;

typedef union {
  AstRoot            root;
  AstBlock           block;
  AstBuiltin         builtin;
  AstTypeFunction    type_function;
  AstTypeSlice       type_slice;
  AstTypeArray       type_array;
  AstDeclaration     declaration;
  AstAssign          assign;
  AstLiteralSequence literal_sequence;
  TokenIndex         literal_int;
  TokenIndex         literal_string;
  TokenIndex         identifier;
  AstFieldAccess     access;
  AstCall            call;
  AstIndexData       index;
  AstUnaryOp         unary_op;
  AstBinaryOp        binary_op;
  AstFunction        function;
  AstParam           param;
  AstIfElse          if_else;
  AstFor             for_;
  AstDefer           defer;
  AstConst           const_;
  AstCast            cast;
} AstNodeData;

typedef struct {
  TokenIndex start;
  TokenIndex end;
} SpanToken;

#define Max_nodes ((usize)1 << 24)

typedef struct {
  Arena kinds;
  Arena spans;
  Arena datas; // holds an offset into `extra` (or maybe use a pointer?)
  Arena types;

  Arena extra;
} AstNodes;

void nodes_init(AstNodes *nodes);
void nodes_deinit(AstNodes *nodes);

#endif // AST_H
