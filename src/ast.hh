enum AstKind : u8 {
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
  Ast_if_else,
  Ast_while,
  Ast_break,
  Ast_continue,
  Ast_return,
  Ast_defer,
  Ast_kind_max,
};

enum BuiltinKind : u8 {
  Builtin_print,
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

struct AstBuiltin {
  BuiltinKind kind;
  union {
    SegmentList<NodeIndex> args;
  };
};

struct AstTypeFunction {
  NodeIndex              return_type;
  SegmentList<NodeIndex> param_types;
};

struct AstTypeSlice {
  NodeIndex base;
};

struct AstTypeArray {
  NodeIndex size;
  NodeIndex base;
};

struct AstDeclaration {
  TokenIndex name;
  NodeIndex  type;
  NodeIndex  value;
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
  SegmentList<NodeIndex> param_names;

  // NOTE: `NodeIndex body` could be removed.
  // You can say that the next node is the body and then you no longer have to
  // explicitly reference it.
  NodeIndex body; // always a block
};

struct AstIfElse {
  NodeIndex         cond;
  NodeIndex         then;
  OptionalNodeIndex otherwise;
};

struct AstWhile {
  NodeIndex cond;
  NodeIndex body;
};

struct AstUnaryOp {
  UnaryOpKind kind;
  NodeIndex   value;
};

struct AstBinaryOp {
  BinaryOpKind kind;
  NodeIndex    lhs;
  NodeIndex    rhs;
};

struct AstFieldAccess {
  NodeIndex base;
  NodeIndex field;
};

struct AstCall {
  NodeIndex              callee;
  SegmentList<NodeIndex> args;
};

struct AstIndex {
  NodeIndex indexable;
  NodeIndex index_at;
};

struct AstCast {
  NodeIndex destination_type;
  NodeIndex value;
};

struct AstReturn {
  NodeIndex value;
};

struct AstDefer {
  NodeIndex value;
};

struct AstAtom {
  TokenIndex token_index;
};

struct AstAssign {
  AssignKind kind;
  NodeIndex  lhs;
  NodeIndex  value;
};

union AstNodeData {
  AstRoot            root;
  AstBlock           block;
  AstBuiltin         builtin;
  AstTypeFunction    type_function;
  AstTypeSlice       type_slice;
  AstTypeArray       type_array;
  AstDeclaration     declaration;
  AstAssign          assign;
  AstLiteralSequence literal_sequence;
  AstAtom            literal_int;
  AstAtom            literal_string;
  AstAtom            identifier;
  AstFieldAccess     access;
  AstCall            call;
  AstIndex           index;
  AstCast            cast;
  AstUnaryOp         unary_op;
  AstBinaryOp        binary_op;
  AstFunction        function;
  AstIfElse          if_else;
  AstWhile           while_;
  AstReturn          return_;
  AstDefer           defer;
};

struct AstNode {
  AstKind          kind;
  Span<TokenIndex> span;
  AstNodeData      data;
};

struct AstNodes {
  Vector<AstKind>          kinds;
  Vector<Span<TokenIndex>> spans;
  Vector<AstNodeData>      datas;

  Allocator segment_allocator;

  void init(Allocator vector_allocator, Allocator segment_allocator) {
    kinds.init(vector_allocator);
    spans.init(vector_allocator);
    datas.init(vector_allocator);

    this->segment_allocator = segment_allocator;
  }

  void deinit() {
    kinds.deinit();
    spans.deinit();
    datas.deinit();

    memset(this, 0, sizeof(*this));
  }

  usize len() { return kinds.len(); }

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

  AstKind          kind(NodeIndex idx) { return kinds[idx.idx]; }
  Span<TokenIndex> span(NodeIndex idx) { return spans[idx.idx]; }
  AstNodeData      data(NodeIndex idx) { return datas[idx.idx]; }
};

constexpr char const *ast_string[Ast_kind_max + 1] = {
  "root",        "block",  "type-slice",       "type-array",  "type-function",  "builtin",
  "declaration", "assign", "literal-sequence", "literal-int", "literal-string", "identifier",
  "call",        "index",  "unary-op",         "binary-op",   "function",       "if-else",
  "while",       "break",  "continue",         "return",      "defer",          "illegal",
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
