#include "option.hh"
#include "arena_item_pool.hh"

struct ValueIndexTag {};
using ValueIndex         = Index<u32, ValueIndexTag>;
using OptionalValueIndex = OptionalIndex<u32, ValueIndexTag>;

union ValuePayload {
  TypeIndex type;
  NodeIndex node_index;
  ValueIndex any;
  i64 int64;
  ValuePayload *items;
  struct {
    i64 len;
    ValuePayload *items;
  } slice;
};

struct Value {
  TypeIndex type;
  ValuePayload data;
};

struct ValueStore {
  ArenaItemPool<Value> pool;
  Allocator blocks_allocator;

  void init(Allocator blocks_allocator) {
    pool.init(MiB(64));
    this->blocks_allocator = blocks_allocator;
  }

  ValueIndex add(Value **out) {
    u32 idx = pool.reserve_index();
    *out    = pool.get(idx);
    return {idx};
  }

  ValuePayload *alloc_data(u32 count) { return blocks_allocator.alloc<ValuePayload>(count); }

  // `drop` frees up the slot in the item pool of values AND frees any payload memory it owns.
  void drop(ValueIndex idx) {
    // TODO
  }

  // `forget` frees up the slot in the item pool of values.
  void forget(ValueIndex idx) {
    // TODO
  }

  Value *get(ValueIndex idx) { return pool.get(idx.idx); }

  // Returns the number of bytes written to `buf`.
  u32 value_to_string(TypeInterner *types, ValueIndex idx, char *buf, u32 buf_size);
};
