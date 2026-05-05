#include "blu.hh"

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
  case Type_integer:
  case Type_literal_int: {
    i64 i     = *cast<i64 *>(v->data);
    offset    = snprintf(buf, buf_size, "%lld", i);
    buf      += offset;
    buf_size -= offset;
    written  += offset;
  } break;
  case Type_literal_function:
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
