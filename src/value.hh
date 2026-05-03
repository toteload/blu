#include "option.hh"
#include "arena_item_pool.hh"

struct ValueIndexTag {};
using ValueIndex         = Index<u32, ValueIndexTag>;
using OptionalValueIndex = OptionalIndex<u32, ValueIndexTag>;

union ValuePayload {
  TypeIndex type;
  NodeIndex node_index;
  i64 int64;

  // Used by array.
  void *memory;

  struct {
    i64 len;
    void *items;
  } slice;

  // The length of the sequence is stored in the type.
  ValueIndex *items;

  struct {
    TypeIndex type;
    u8 *memory;
  } any;
};

struct Value {
  TypeIndex    type;
  ValuePayload data;
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

  Value *get(ValueIndex idx) { return values.get(idx.idx); }

  ValueIndex alloc_sequence(TypeIndex type, u32 length, ValueIndex **items);
  ValueIndex alloc_slice(TypeIndex type, u32 length, ValueIndex **items);

  ValueIndex alloc_with_memory(TypeIndex type, u32 byte_count, u32 align, u8 **out);

  // Returns the number of bytes written to `buf`.
  u32 value_to_string(TypeInterner *types, ValueIndex idx, char *buf, u32 buf_size);
};
