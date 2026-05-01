#include "option.hh"
#include "arena_item_pool.hh"

struct ValueIndexTag {};
using ValueIndex         = Index<u32, ValueIndexTag>;
using OptionalValueIndex = OptionalIndex<u32, ValueIndexTag>;

struct Value {
  TypeIndex type;

  union {
    TypeIndex type;
    NodeIndex node_index;
    i64 int64;
    u8 *memory;
  } data;
};

struct ValueStore {
  ArenaItemPool<Value> values;
  Allocator blocks_allocator;

  void init(Allocator blocks_allocator) {
    values.init(MiB(64));
    this->blocks_allocator = blocks_allocator;
  }

  ValueIndex add(Value const &val) {
    ValueIndex idx = {values.reserve_index()};
    Value *p       = get(idx);
    *p             = val;
    return idx;
  }

  ValueIndex add_with_memory(TypeIndex type, u32 byte_size, u32 align, u8 **out);

  Value *get(ValueIndex idx) { return values.get(idx.idx); }

  // Returns the number of bytes written to `buf`.
  u32 value_to_string(TypeInterner *types, ValueIndex idx, char *buf, u32 buf_size);
};
