#ifndef AST_H
#define AST_H

#include "blu.h"
#include "tokens.h"

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

typedef struct {
  u8 kind;
  u32 count;
  AstIndex args[];
} AstBuiltin;

typedef struct {
  AstIndex     return_type;
  u32 count;
  AstIndex param_types[];
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
  u32 count;
  AstIndex items[];
} AstSource;

typedef struct {
  AstIndex name;
  u32 count;
  AstIndex items[];
} AstModSection;

typedef struct {
  u32 count;
  AstIndex items[];
} AstBlock;

typedef struct {
  u32 count;
  AstIndex items[];
} AstLiteralSequence;

typedef struct {
  TokenIndex name;
  AstIndex  type;
} AstParam;

typedef struct {
  AstIndex return_type;
  AstIndex body;
  u32 count;
  AstIndex params[];
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
  AstIndex callee;
  u32 count;
  AstIndex args[];
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

String ast_kind_string(u8 kind);

typedef struct {
  MessageSink *msg_sink;
  Arena *arena;
  Arena *scratch;
} ParseContext;

// Index 0 is reserved so that 0 can be used as 'no node'; the root node is at index 1.
// `datas` holds a u32 byte offset into `extra` per node; the u32s are essentially compressed
// pointers into the arena that backs the AST. Payloads are stored packed in `extra`; nodes with
// variable-length children store them as a trailing array in their payload, which means a payload
// is only written once the node is complete, so children appear before their parent in `extra`.
typedef struct {
  u32        count;
  u8        *kinds;
  SpanToken *spans;
  u32       *datas;
  void      *extra;
} AstNodes2;

void *ast_data(AstNodes2 *ast, AstIndex idx);

b32 parse(ParseContext *context, Tokens *tokens, AstNodes2 *ast);

#endif // AST_H
