#include "option.hh"
#include "arena_item_pool.hh"

struct ValueIndexTag {};
using ValueIndex         = Index<u32, ValueIndexTag>;
using OptionalValueIndex = OptionalIndex<u32, ValueIndexTag>;

struct ValueSlice {
  u64 len;
  void *items;
};

struct Value {
  TypeIndex type;
  void *data;
};

struct ValueStore {
  ArenaItemPool<Value> pool;
  Allocator payload_allocator;

  void init(Allocator payload_allocator) {
    pool.init(MiB(64));
    this->payload_allocator = payload_allocator;
  }

  ValueIndex alloc_value(Value **out) {
    u32 idx = pool.reserve_index();
    *out    = pool.get(idx);
    return {idx};
  }

  // TODO: maybe it doesn't make sense that this function has a count parameter and that information
  // should just be encoded in the TypeSizeInfo.
  void *alloc_memory(TypeSizeInfo info, u32 count=1) {
    usize byte_size;
    if (count == 1) {
      byte_size = info.size;
    } else {
      byte_size = info.stride * count;
    }

    return payload_allocator.raw_alloc(byte_size, info.align);
  }

  template<typename T> T *alloc_data(u32 count = 1) {
    return cast<T *>(alloc_memory(TypeSizeInfo::of_type<T>(), count));
  }

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
