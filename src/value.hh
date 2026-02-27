#include "option.hh"

struct ValueIndexTag {};

using ValueIndex         = Index<u32, ValueIndexTag>;
using OptionalValueIndex = OptionalIndex<u32, ValueIndexTag>;

enum ValueKind : u8 {
  Value_true,
  Value_false,

  Value_type,

  Value_param,
  Value_declaration,

  Value_hir,

  Value_lazy_value,
};

struct LazyValue {
  NodeIndex node_index;
  OptionalValueIndex val;

  static LazyValue from_node_index(NodeIndex x) {
    return {
      x,
      OptionalValueIndex::none(),
    };
  }

  bool is_evaluated() { return val.is_some(); }
  void set(ValueIndex x) { val = OptionalValueIndex::from_index(x); }
  ValueIndex get() { return val.as_index(); }
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
    return {
      Value_type,
      {
        .type = type,
      }
    };
  }

  static Value make_param(TokenIndex param, TypeIndex type) {
    return {
      Value_param,
      {
        .param = {
          .token_index = param,
          .type        = type,
        },
      },
    };
  }

  static Value make_declaration(NodeIndex declared_type, NodeIndex value) {
    return {
      Value_declaration,
      {
        .declaration = {
          LazyValue::from_node_index(declared_type),
          LazyValue::from_node_index(value),
        },
      },
    };
  }

  static Value make_lazy_value(NodeIndex node_index) {
    return {
      Value_lazy_value,
      {
        .lazy_value = LazyValue::from_node_index(node_index),
      },
    };
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
