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

enum AstKind : u8 {
  Ast_module,

  Ast_param,
  Ast_function,

  Ast_scope,

  Ast_identifier,
  Ast_literal_int,
  Ast_literal_string,

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

  Ast_builtin,

  Ast_kind_max,
};

enum BuiltinKind : u8 {
  Builtin_import,
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
    } builtin;

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
