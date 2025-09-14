enum AstKind : u8 {
  Ast_root,
  Ast_block,
  Ast_type,
  Ast_declaration,
  Ast_assign,
  Ast_literal_sequence,
  Ast_literal_int,
  Ast_identifier,
  Ast_field_access,
  Ast_call,
  Ast_cast,
  Ast_unary_op,
  Ast_binary_op,
  Ast_function,
  Ast_if_else,
  Ast_while,
  Ast_break,
  Ast_continue,
  Ast_return,
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

  BinaryOpKind_max,
};

enum AssignKind : u8 {
  Assign_normal,

  AssignKind_max,
};

enum UnaryOpKind : u8 {
  Negate,
  Not,

  UnaryOpKind_max,
};

enum AstTypeKind {
  Ast_type_identifier,
  Ast_type_slice,
  Ast_type_function,
};

enum TypeFlag : u32 {
  None     = 0,
  Distinct = 1 >> 0,
};

struct OptionalNodeIndex;

struct NodeIndex {
  u32 idx;

  ttld_inline static NodeIndex from_optional(OptionalNodeIndex i);
};

struct OptionalNodeIndex {
  u32 idx;

  static OptionalNodeIndex from_node_index(NodeIndex ni) { return {ni.idx}; }

  b32 is_none() { return idx == UINT32_MAX; }
};

NodeIndex NodeIndex::from_optional(OptionalNodeIndex i) {
  Debug_assert(!i.is_none());
  return {i.idx};
}

constexpr OptionalNodeIndex OptionalNodeIndex_none = {UINT32_MAX};

struct AstNode;

struct AstType {
  AstTypeKind kind;
  u32 flags;
  union {
    TokenIndex token_index;
    NodeIndex base;
    struct {
      NodeIndex return_type;
      SegmentList<NodeIndex> params;
    } function;
  } data;
};

struct Param {
  TokenIndex name;
  NodeIndex type;
};

struct Declaration {
  TokenIndex name;
  NodeIndex type;
  NodeIndex value;
};

struct Root {
  SegmentList<NodeIndex> items;
};

struct Block {
  SegmentList<NodeIndex> items;
};

struct LiteralSequence {
  SegmentList<NodeIndex> items;
};

struct Function {
  SegmentList<Param> params;
  NodeIndex return_type;
  NodeIndex body;
};

struct IfElse {
  NodeIndex cond;
  NodeIndex then;
  OptionalNodeIndex otherwise;
};

struct While {
  NodeIndex cond;
  NodeIndex body;
};

struct UnaryOp {
  UnaryOpKind kind;
  NodeIndex value;
};

struct BinaryOp {
  BinaryOpKind kind;
  NodeIndex lhs;
  NodeIndex rhs;
};

struct FieldAccess {
  NodeIndex base;
  NodeIndex field;
};

struct Call {
  NodeIndex callee;
  SegmentList<NodeIndex> args;
};

struct Cast {
  NodeIndex destination_type;
  NodeIndex value;
};

struct Return {
  NodeIndex value;
};

struct Atom {
  TokenIndex token_index;
};

struct Assign {
  AssignKind kind;
  NodeIndex lhs;
  NodeIndex value;
};

union NodeData {
  Root root;
  Block block;
  AstType type;
  Declaration declaration;
  Assign assign;
  LiteralSequence literal_sequence;
  Atom literal_int;
  Atom identifier;
  FieldAccess access;
  Call call;
  Cast cast;
  UnaryOp unary_op;
  BinaryOp binary_op;
  Function function;
  IfElse if_else;
  While while_;
  Return return_;
};

struct Nodes {
  Vector<AstKind> kinds;
  Vector<Span<TokenIndex>> spans;
  Vector<NodeData> datas;

  Allocator segment_allocator;

  NodeIndex alloc() {
    NodeIndex res = {cast<u32>(kinds.len())};
    kinds.push_empty();
    spans.push_empty();
    datas.push_empty();
    return res;
  }

  AstKind &kind(NodeIndex idx) { return kinds[idx.idx]; }
  Span<TokenIndex> &span(NodeIndex idx) { return spans[idx.idx]; }
  NodeData &data(NodeIndex idx) { return datas[idx.idx]; }
};

constexpr char const *ast_string[Ast_kind_max + 1] = {
  "root",        "block",      "type",         "declaration", "literal-sequence",
  "literal-int", "identifier", "field-access", "call",        "cast",
  "unary-op",    "binary-op",  "function",     "if-else",     "while",
  "break",       "continue",   "return",       "illegal",
};

ttld_inline char const *ast_kind_string(u32 kind) {
  if (kind >= Ast_kind_max) {
    return ast_string[Ast_kind_max];
  }

  return ast_string[kind];
}

constexpr char const *binary_op_string[BinaryOpKind_max + 1] = {
  "* (Mul)",
  "/ (Div)",
  "% (Mod)",
  "- (Sub)",
  "+ (Add)",
  "<< (Bit_shift_left)",
  ">> (Bit_shift_right)",
  "& (Bit_and)",
  "| (Bit_or)",
  "~ (Bit_xor)",
  "== (CmpEq)",
  "!= (CmpNe)",
  "> (CmpGt)",
  ">= (CmpGe)",
  "< (CmpLt)",
  "<= (CmpLe)",
  "and (Logical_and)",
  "or (Logical_or)",
  "illegal",
};

ttld_inline char const *binary_op_kind_string(u32 kind) {
  if (kind >= BinaryOpKind_max) {
    return binary_op_string[BinaryOpKind_max];
  }

  return binary_op_string[kind];
}
