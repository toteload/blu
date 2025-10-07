#include "option.hh"

struct ValueIndex {
  u32 idx = UINT32_MAX;
};

enum ValueKind : u8 {
  Value_true,
  Value_false,

  Value_type,

  Value_param,
  Value_declaration,

  Value_lazy_type,
  Value_lazy_value,
};

struct LazyValue {
  NodeIndex node_index;
  Option<ValueIndex> val;

  static LazyValue from_node_index(NodeIndex x) {
    return {
      x,
      Option<ValueIndex>(),
    };
  }

  bool is_evaluated() { return val.is_some(); }
  void set(ValueIndex x) { val.set(x); }
  ValueIndex get() { return val.get(); }
};

struct Value {
  ValueKind kind;

  struct Param {
    TokenIndex token_index;
    TypeIndex type;
  };

  struct Declaration {
    LazyValue type;
    LazyValue value;
  };

  union {
    Param param;
    Declaration declaration;
    TypeIndex type;
    LazyValue lazy_value;
  } data;

  static Value make_type(TypeIndex type) {
    Value val = {
      Value_type,
      {
        .type = type,
      }
    };
    return val;
  }

  static Value make_param(TokenIndex param, TypeIndex type) {
    Value val = {
      Value_param,
      {
        .param = {
          .token_index = param,
          .type        = type,
        },
      },
    };
    return val;
  }

  static Value make_declaration(NodeIndex declared_type, NodeIndex value) {
    Value val = {
      Value_declaration,
      {
        .declaration = {
          LazyValue::from_node_index(declared_type),
          LazyValue::from_node_index(value),
        },
      },
    };
    return val;
  }

  static Value lazy_type(NodeIndex node_index) {
    Value val = {
      Value_lazy_type,
      {
        .lazy_value = LazyValue::from_node_index(node_index),
      },
    };
    return val;
  }
};

struct ValueStore {
  Vector<Value> values;

  void init(Allocator allocator) { values.init(allocator); }

  void deinit();

  ValueIndex add(Value val) {
    u32 idx = cast<u32>(values.len());
    values.push(val);
    return {idx};
  }

  // Careful! This pointer is not stable.
  // If you add another value, this pointer may get invalidated.
  Value *get(ValueIndex idx) { return &values[idx.idx]; }
};
