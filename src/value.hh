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
  Value_sequence,
};

struct Value {
  ValueKind kind;
  TypeIndex type;

  union {
    TypeIndex type;
    NodeIndex node_index;
    i64 int64;

    struct {
	    Slice<Value> items;
    } sequence;
  } data;
};

struct ValueStore {
  ArenaItemPool<Value> values;
  Allocator value_blocks; todo

  void init() { values.init(MiB(64)); }

  void deinit();

  ValueIndex add(Value const &val) {
    ValueIndex idx = {values.reserve_index()};
    Value *p       = get(idx);
    *p             = val;
    return idx;
  }

  Value *get(ValueIndex idx) { return values.get(idx.idx); }

  // Returns the number of bytes written to `buf`.
  u32 value_to_string(TypeInterner *types, ValueIndex idx, char *buf, u32 buf_size);
};
