enum AstKind : u8 {
  Ast_root,
  Ast_block,
  Ast_type_slice,
  Ast_type_function,
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

struct AstNodeTag {};

using NodeIndex         = Index<u32, AstNodeTag>;
using OptionalNodeIndex = OptionalIndex<u32, AstNodeTag>;

struct AstTypeFunction {
  NodeIndex return_type;
  SegmentList<NodeIndex> params;
};

struct AstTypeSlice {
  NodeIndex base;
};

struct AstParam {
  TokenIndex name;
  NodeIndex type;
};

struct AstDeclaration {
  TokenIndex name;
  NodeIndex type;
  NodeIndex value;
};

struct AstRoot {
  SegmentList<NodeIndex> items;
};

struct AstBlock {
  SegmentList<NodeIndex> items;
};

struct AstLiteralSequence {
  SegmentList<NodeIndex> items;
};

struct AstFunction {
  SegmentList<AstParam> params;
  NodeIndex return_type;
  NodeIndex body;
};

struct AstIfElse {
  NodeIndex cond;
  NodeIndex then;
  OptionalNodeIndex otherwise;
};

struct AstWhile {
  NodeIndex cond;
  NodeIndex body;
};

struct AstUnaryOp {
  UnaryOpKind kind;
  NodeIndex value;
};

struct AstBinaryOp {
  BinaryOpKind kind;
  NodeIndex lhs;
  NodeIndex rhs;
};

struct AstFieldAccess {
  NodeIndex base;
  NodeIndex field;
};

struct AstCall {
  NodeIndex callee;
  SegmentList<NodeIndex> args;
};

struct AstCast {
  NodeIndex destination_type;
  NodeIndex value;
};

struct AstReturn {
  NodeIndex value;
};

struct AstAtom {
  TokenIndex token_index;
};

struct AstAssign {
  AssignKind kind;
  NodeIndex lhs;
  NodeIndex value;
};

union AstNodeData {
  AstRoot root;
  AstBlock block;
  AstTypeFunction type_function;
  AstTypeSlice type_slice;
  AstDeclaration declaration;
  AstAssign assign;
  AstLiteralSequence literal_sequence;
  AstAtom literal_int;
  AstAtom identifier;
  AstFieldAccess access;
  AstCall call;
  AstCast cast;
  AstUnaryOp unary_op;
  AstBinaryOp binary_op;
  AstFunction function;
  AstIfElse if_else;
  AstWhile while_;
  AstReturn return_;
};

struct AstNode {
  AstKind kind;
  Span<TokenIndex> span;
  AstNodeData data;
};

struct AstNodes {
  Vector<AstKind> kinds;
  Vector<Span<TokenIndex>> spans;
  Vector<AstNodeData> datas;

  Allocator segment_allocator;

  NodeIndex alloc() {
    NodeIndex res = {cast<u32>(kinds.len())};
    kinds.push_empty();
    spans.push_empty();
    datas.push_empty();
    return res;
  }

  void set(NodeIndex idx, AstNode node) {
    kinds[idx.idx] = node.kind;
    spans[idx.idx] = node.span;
    datas[idx.idx] = node.data;
  }

  NodeIndex add(AstNode node) {
    NodeIndex res = {cast<u32>(kinds.len())};
    kinds.push(node.kind);
    spans.push(node.span);
    datas.push(node.data);
    return res;
  }

  AstKind kind(NodeIndex idx) { return kinds[idx.idx]; }
  Span<TokenIndex> span(NodeIndex idx) { return spans[idx.idx]; }
  AstNodeData data(NodeIndex idx) { return datas[idx.idx]; }
};

constexpr char const *ast_string[Ast_kind_max + 1] = {
  "root",        "block",      "type-slice",   "type-function", "declaration", "literal-sequence",
  "literal-int", "identifier", "field-access", "call",          "cast",        "unary-op",
  "binary-op",   "function",   "if-else",      "while",         "break",       "continue",
  "return",      "illegal",
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
