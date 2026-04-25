#include "option.hh"
#include "arena_item_pool.hh"

struct ValueIndexTag {};
using ValueIndex         = Index<u32, ValueIndexTag>;
using OptionalValueIndex = OptionalIndex<u32, ValueIndexTag>;

enum ValueKind : u8 {
  Val_true,
  Val_false,

  Val_type,

  Val_int,
  Val_function,
};

struct Value {
  ValueKind kind;
  TypeIndex type;

  union {
    TypeIndex type;
  } data;
};

struct ValueStore {
  ArenaItemPool<Value> values;

  void init() { values.init(MiB(64)); }

  void deinit();

  ValueIndex add(Value const &val) {
    ValueIndex idx = {values.reserve_index()};
    Value *p       = get(idx);
    *p             = val;
    return idx;
  }

  Value *get(ValueIndex idx) { return values.get(idx.idx); }
};
