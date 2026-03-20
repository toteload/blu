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

  Value_lazy,
};

struct Value {
  ValueKind kind;

  struct Param {
    TokenIndex token_index;
    TypeIndex type;
  };

  union {
    Param param;
    TypeIndex type;
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
