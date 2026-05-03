#include "blu.hh"

ValueIndex ValueStore::alloc_with_items(TypeIndex type, u32 count, ValueIndex **items) {
  auto p = blocks_allocator.alloc<ValueIndex>(count);
  auto idx = add({ .type = type, .data = { .items = p, }, });
  *items = p;
  return idx;
}

ValueIndex ValueStore::alloc_slice(TypeIndex slice_type, u32 length, ValueIndex **items) {
  auto p = blocks_allocator.alloc<ValueIndex>(length);
  auto idx = add({ .type = slice_type, .data = { .slice = { .len = length, .items = p, }, }, });
  *items = p;
  return idx;
}

ValueIndex ValueStore::alloc_with_memory(TypeIndex type, u32 byte_count, u32 align, u8 **out) {
  u8 *p = cast<u8*>(blocks_allocator.raw_alloc(byte_count, align));
  auto idx = add({ .type = type, .data = { .memory = p, }, });
  *out = p;
  return idx;
}

u32 ValueStore::value_to_string(TypeInterner *types, ValueIndex idx, char *buf, u32 buf_size) {
  Value *v = get(idx);

  u32 written = 0;
  u32 offset;

  offset    = snprintf(buf, buf_size, "(");
  buf      += offset;
  buf_size -= offset;
  written  += offset;

  offset    = type_to_string(types, v->type, buf, buf_size);
  buf      += offset;
  buf_size -= offset;
  written  += offset;

  offset    = snprintf(buf, buf_size, ") ");
  buf      += offset;
  buf_size -= offset;
  written  += offset;

  auto t = types->get(v->type);

  switch (t->kind) {
  case Type_literal_int: {

    offset    = snprintf(buf, buf_size, "%lld", v->data.int64);
    buf      += offset;
    buf_size -= offset;
    written  += offset;

  } break;
  case Type_literal_function:
  case Type_integer:
  case Type_boolean:
  case Type_function:
  case Type_nil:
  case Type_never:
  case Type_slice:
  case Type_array:
  case Type_distinct:
  case Type_sequence:
  case Type_type:
    Todo();
    break;
  }

  return written;
}
