enum ValueKind : u8 {
  Value_builtin,
  Value_param,
  Value_local,
  Value_iter_item,
};

struct Value {
  ValueKind kind;
  Type *type;

  union {
    struct {
      AstNode *ast;
    } param;

    struct {
      AstNode *ast;
    } local;

    struct {
      AstNode *ast;
    } iter_item;
  };

  static Value make_param(AstNode *ast, Type *type) {
    Value val;
    val.kind       = Value_param;
    val.type = type;
    val.param.ast  = ast;
    return val;
  }

  static Value make_local(AstNode *ast, Type *type) {
    Value val;
    val.kind       = Value_local;
    val.type = type;
    val.param.ast  = ast;
    return val;
  }

  static Value make_builtin(Type *type) {
    Value val;
    val.kind         = Value_builtin;
    val.type = type;
    return val;
  }

  static Value make_iter_item(Type *type, AstNode *n) {
    Value val;
    val.kind           = Value_iter_item;
    val.type = type;
    val.iter_item.ast  = n;
    return val;
  }
};

