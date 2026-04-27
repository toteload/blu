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

  switch (v->kind) {
  case Val_int: {
    offset    = snprintf(buf, buf_size, "%lld", v->data.int64);
    buf      += offset;
    buf_size -= offset;
    written  += offset;
  } break;
  default:
    Todo();
  }

  return written;
}
