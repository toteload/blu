#ifndef AST_H
#define AST_H

#include "blu.h"

enum AstKind {
  Ast_source,
  Ast_mod_section,
  Ast_block,

  Ast_type_slice,
  Ast_type_array,
  Ast_type_function,

  Ast_builtin,

  Ast_declaration,
  Ast_assign,
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
  Ast_as,

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

enum AttributeFlag {
  Attribute_comptime = 1 << 0,
  Attribute_no_cache = 1 << 1,
};

#define SEGMENTLIST_NAME          AstIndexList
#define SEGMENTLIST_TYPE          AstIndex
#define SEGMENTLIST_MIN_SIZE_LOG2 3
#define SEGMENTLIST_SEGMENT_COUNT 24
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

typedef struct {
  u8 kind;
  AstIndexList args;
} AstBuiltin;

typedef struct {
  AstIndex     return_type;
  AstIndexList param_types;
} AstTypeFunction;

typedef struct {
  AstIndex base;
} AstTypeSlice;

typedef struct {
  AstIndex size;
  AstIndex base;
} AstTypeArray;

typedef struct {
  TokenIndex name;
  AstIndex  type;
  AstIndex  value;
} AstDeclaration;

typedef struct {
  AstIndexList items;
} AstSource;

typedef struct {
  AstIndex     name;
  AstIndexList items;
} AstModSection;

typedef struct {
  AstIndexList items;
} AstBlock;

typedef struct {
  AstIndexList items;
} AstLiteralSequence;

typedef struct {
  TokenIndex name;
  AstIndex  type;
} AstParam;

typedef struct {
  AstIndexList params;
  AstIndex     return_type;
  AstIndex     body;
} AstFunction;

typedef struct {
  AstIndex cond;
  AstIndex then;
  AstIndex otherwise;
} AstIfElse;

typedef struct {
  AstIndex iterable;
  AstIndex iterator;
  AstIndex body;
} AstFor;

typedef struct {
  u8 op_kind;
  AstIndex value;
} AstUnaryOp;

typedef struct {
  u8 op_kind;
  AstIndex    lhs;
  AstIndex    rhs;
} AstBinaryOp;

typedef struct {
  AstIndex base;
  AstIndex field;
} AstFieldAccess;

typedef struct {
  AstIndex     callee;
  AstIndexList args;
} AstCall;

typedef struct {
  AstIndex indexable;
  AstIndex index_at;
} AstIndexData;

typedef struct {
  AstIndex value;
} AstDefer;

typedef struct {
  u8 assign_kind;
  AstIndex  lhs;
  AstIndex  value;
} AstAssign;

typedef struct {
  AstIndex expr;
} AstConst;

typedef struct {
  AstIndex type_dst;
  AstIndex value;
} AstCast;

typedef struct {
  AstIndex type_dst;
  AstIndex value;
} AstAs;

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
  AstAs              as;
} AstNodeData;

typedef struct {
  TokenIndex start;
  TokenIndex end;
} SpanToken;

// If you are going to refactor this such that it only uses one backing Arena, like Tokens, then
// you still want to keep a pointer around to the arena for when you want to allocate 'extra'
// memory. The extra data that `datas` refers to won't be packed anymore like it is now, but that
// seems fine. The u32s will essentially be compressed pointers.
typedef struct {
  Arena kinds; // u8[]
  Arena spans; // SpanToken[]
  Arena datas; // holds a u32 offset into `extra`
  Arena extra;
  u32   offset;
} AstNodes;

void nodes_init(AstNodes *nodes);
void nodes_deinit(AstNodes *nodes);

AstIndex nodes_begin(AstNodes *nodes);
AstIndex nodes_end(AstNodes *nodes);

AstIndex   nodes_alloc(AstNodes *nodes);
u8        *nodes_kind(AstNodes *nodes, AstIndex idx);
SpanToken *nodes_span(AstNodes *nodes, AstIndex idx);
void      *nodes_data(AstNodes *nodes, AstIndex idx);

String ast_kind_string(u8 kind);

#endif // AST_H
