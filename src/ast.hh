enum AstKind : u8 {
  Ast_module,

  Ast_type,

  Ast_param,
  Ast_function,

  Ast_scope,

  Ast_identifier,

  Ast_literal_int,
  Ast_literal_string,
  Ast_literal_tuple,

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

  Ast_deref,

  Ast_return,

  Ast_builtin,

  Ast_field_access,

  Ast_kind_max,
};

enum BinaryOpKind : u8 {
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

enum UnaryOpKind : u8 {
  Negate,
  Not,
  AddressOf,

  UnaryOpKind_max,
};

enum BuiltinKind : u8 {
  Builtin_include,
};

enum AstTypeKind {
  Ast_type_identifier,
  Ast_type_pointer,
  Ast_type_slice,
};

enum TypeFlag : u32 {
  Distinct = 1 >> 0,
};

struct AstNode {
  AstKind kind;

  struct {
    u32 start;
    u32 end;
  } token_span;

  Type *type    = nullptr;
  AstNode *next = nullptr;

  union {
    struct {
      BuiltinKind kind;
      AstNode *value;

      union {
        SourceIdx src_idx;
      };
    } builtin;

    struct {
      AstNode *base;
      AstNode *field;
    } field_access;

    struct {
      AstTypeKind kind;
      u32 flags;
      AstNode *base;
    } ast_type;

    struct {
      AstNode *declared_type;
      AstNode *items;
    } tuple;

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
    } return_;

    struct {
      StrKey key;
    } identifier;

    struct {
      AstNode *cond;
      AstNode *body;
    } while_;

    struct {
      AstNode *item;
      AstNode *iterable;
      AstNode *body;
    } for_;

    struct {
      AstNode *expressions;
    } scope;

    struct {
      AstNode *items;
    } module;

    struct {
      AstNode *value;
    } deref;
  };
};

constexpr char const *ast_string[Ast_kind_max + 1] = {
  "module",     "type",        "param",          "function",      "scope",
  "identifier", "literal-int", "literal-string", "literal-tuple", "declaration",
  "assign",     "while",       "break",          "continue",      "for",
  "call",       "if-else",     "binary-op",      "unary-op",      "cast",
  "deref",      "return",      "builtin",        "field_access",  "illegal",
};

ttld_inline char const *ast_kind_string(u32 kind) {
  if (kind >= Ast_kind_max) {
    return ast_string[Ast_kind_max];
  }

  return ast_string[kind];
}

constexpr char const *binary_op_string[BinaryOpKind_max + 1] = {
  "* (Mul)",     "/ (Div)",           "% (Mod)",
  "- (Sub)",     "+ (Add)",           "<<",
  ">>",          "& (Bit_and)",       "| (Bit_or)",
  "^ (Bit_xor)", "== (CmpEq)",        "!= (CmpNe)",
  "> (CmpGt)",   ">= (CmpGe)",        "< (CmpLt)",
  "<= (CmpLe)",  "and (Logical_and)", "or (Logical_or)",
  "(Assign)",    "+= (AddAssign)",    "illegal",
};

ttld_inline char const *binary_op_kind_string(u32 kind) {
  if (kind >= BinaryOpKind_max) {
    return binary_op_string[BinaryOpKind_max];
  }

  return binary_op_string[kind];
}
