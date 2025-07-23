enum ValueKind : u8 {
  Value_type,

  Value_param,
  Value_local,

  Value_iter_item,

  Value_builtin,
};

struct Value {
  ValueKind kind;

  union {
    Type *type;

    struct {
      Type *type;
      AstNode *ast;
    } param;

    struct {
      Type *type;
      AstNode *ast;
    } local;

    struct {
      Type *type;
    } builtin;

    struct {
      Type *type;
      AstNode *ast;
    } iter_item;
  };

  static Value make_type(Type *type) {
    Value val;
    val.kind = Value_type;
    val.type = type;
    return val;
  }

  static Value make_param(AstNode *ast, Type *type) {
    Value val;
    val.kind       = Value_param;
    val.param.ast  = ast;
    val.param.type = type;
    return val;
  }

  static Value make_local(AstNode *ast, Type *type) {
    Value val;
    val.kind       = Value_local;
    val.param.ast  = ast;
    val.param.type = type;
    return val;
  }

  static Value make_builtin(Type *type) {
    Value val;
    val.kind         = Value_builtin;
    val.builtin.type = type;
    return val;
  }

  static Value make_iter_item(Type *type, AstNode *n) {
    Value val;
    val.kind           = Value_iter_item;
    val.iter_item.type = type;
    val.iter_item.ast  = n;
    return val;
  }
};

// This makes sure that no matter variant Value is, you can always access the type of the value
// through the type field.
// This may not be a good idea :)
//static_assert(offsetof(Value, type) == offsetof(Value, param.type));
//static_assert(offsetof(Value, type) == offsetof(Value, local.type));
