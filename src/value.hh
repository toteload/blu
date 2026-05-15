#include "arena_item_pool.hh"

struct ValueSlice {
  u64   len;
  void *items;
};

struct Value {
  TypeIndex type;
  void     *data;
};

struct ValueStore {
  ArenaItemPool<Value> pool;
  Allocator            payload_allocator;

  void init(Allocator payload_allocator) {
    pool.init(MiB(64));
    this->payload_allocator = payload_allocator;
  }

  void deinit() {
    pool.deinit();
    memset(this, 0, sizeof(*this));
  }

  ValueIndex alloc_value(Value **out) {
    u32 idx = pool.reserve_index();
    *out    = pool.get(idx);
    return {idx};
  }

  // TODO: maybe it doesn't make sense that this function has a count parameter and that information
  // should just be encoded in the TypeSizeInfo.
  void *alloc_data(TypeSizeInfo info, u32 count = 1) {
    usize byte_size;
    if (count == 1) {
      byte_size = info.size;
    } else {
      byte_size = info.stride * count;
    }

    return payload_allocator.raw_alloc(byte_size, info.align);
  }

  template<typename T> T *alloc_data(u32 count = 1) {
    return cast<T *>(alloc_data(TypeSizeInfo::of_type<T>(), count));
  }

  Value *get(ValueIndex idx) { return pool.get(idx.idx); }

  // Returns the number of bytes written to `buf`.
  u32 value_to_string(TypeInterner *types, ValueIndex idx, char *buf, u32 buf_size);
};
