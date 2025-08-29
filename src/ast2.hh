enum NodeKind : u8 {
  // main_token is the first token.
  // Uses extra_slice.
  // - extra stores a NodeIndex for each declaration in the root.
  Ast_root,

  // main_token is the name of the declaration.
  // Uses node_and_node
  // - NodeIndex for the type of the declaration
  // - NodeIndex for the initial value of the declaration
  Ast_declaration,

  // Does not use data
  Ast_literal_int,

  // Does not use data
  Ast_literal_string,

  // Does not use data
  Ast_identifier,

  // main_token is '{'
  // Uses extra_slice
  // - extra stores a NodeIndex for each expression in the block.
  Ast_block,

  // main_token is 'distinct'
  // Uses node for the type expression.
  Ast_distinct_type,

  // main_token is 'fn'.
  // Uses extra_and_node.
  // - extra stores an ExtraSlice at extra_idx. 
  //   The ExtraSlice holds a NodeIndex for each parameter type.
  // - NodeIndex for the return type of the function.
  Ast_function_type,

  // main_token is 'fn'
  // Uses node_and_node.
  // - node_and_node.a_idx to `Ast_function_type'
  // - node_and_node.b_idx to `Ast_block`
  Ast_function,

  // main_token is 'return'
  // Uses node.
  // - node_idx to return value expression
  Ast_return,

  // main_token is '<='
  // Uses node_and_node.
  Ast_cmp_less_than,

  // main_token is 'if'
  // Uses node_and_extra.
  // - node_and_extra.node_idx to the condition expression.
  // - node_and_extra.extra_idx to `IfPayload`.
  Ast_if_else,

  // main_token is '('
  // Uses node_and_extra
  // - node_and_extra.node_idx to the function expression.
  // - node_and_extra.extra_idx to 'ExtraSlice'.
  //   The 'ExtraSlice' has a NodeIndex for each argument.
  Ast_call,
  
  // main_token is 'continue'
  Ast_continue,

  // main_token is 'break'
  Ast_break,

  // main_token is 'while'
  // Uses node_and_node
  // - node_and_node.a_idx to the condition expression
  // - node_and_node.b_idx to the body
  Ast_while,

  // main_token is '='
  // Uses node_and_node.
  // - node_and_node.a_idx to the L-value expression
  // - node_and_node.b_idx to the rhs expression
  Ast_assign,
};

struct NodeIndex {
  u32 idx;
};

// Optional value is UINT32_MAX
struct OptionalNodeIndex {
  u32 idx;
};

struct TokenIndex {
  u32 idx;
};

struct ExtraIndex {
  u32 idx;
};

struct ExtraSlice {
  u32 start;
  u32 end;
};

struct IfPayload {
  NodeIndex         then;
  OptionalNodeIndex otherwise;
};

union NodeData {
  NodeIndex node_idx;

  struct {
    NodeIndex a;
    NodeIndex b;
  } node_and_node;

  struct {
    ExtraIndex extra_idx;
    NodeIndex node_idx;
  } extra_and_node;

  ExtraSlice extra_slice;
};

struct Nodes {
  Vector<NodeKind> kind;
  Vector<NodeData> data;
  Vector<TokenIndex> main_token;

  // -

  Vector<u32> extra;
};

struct Ast {
  Str source;
  Tokens tokens;
  Nodes nodes;
};

