enum ValueKind : u8 {
  Value_primitive_type,
  Value_param,
  Value_lazy_declaration,
};

struct Value {
  ValueKind kind;
  Type *type = nullptr;

  struct Param {
    TokenIndex token_index;
  };

  struct Declaration {
    NodeIndex type_node_index;
    NodeIndex value_node_index;
  };

  union {
    Param param;
    Declaration declaration;
  } data;

  b32 is_type_known() { return type != nullptr; }

  static Value make_primitive_type(Type *type) {
    Value val;
    val.kind       = Value_primitive_type;
    val.type = type;
    return val;
  }

  static Value make_param(TokenIndex param, Type *type) {
    Value val;
    val.kind       = Value_param;
    val.type = type;
    val.data.param.token_index = param;
    return val;
  }

  static Value make_declaration(NodeIndex declared_type, NodeIndex value) {
    Value val;
    val.kind = Value_lazy_declaration;
    val.data.declaration.type_node_index = declared_type;
    val.data.declaration.value_node_index = value;
    return val;
  }
};

