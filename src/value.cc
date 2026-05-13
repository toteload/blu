#include "blu.hh"

u32 ValueStore::value_to_string(TypeInterner *types, ValueIndex idx, char *buf, u32 buf_size) {
  Value *v = get(idx);

  u32 written = 0;
  u32 offset;

#if 0
  offset    = snprintf(buf, buf_size, "(");
  buf      += offset;
  buf_size -= offset;
  written  += offset;

  offset    = types->type_to_string(v->type, buf, buf_size);
  buf      += offset;
  buf_size -= offset;
  written  += offset;

  offset    = snprintf(buf, buf_size, ")");
  buf      += offset;
  buf_size -= offset;
  written  += offset;
#endif

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
  case Type_boolean: { Todo(); } break;
  case Type_literal_function: { Todo(); } break;
  case Type_function: { written += snprintf(buf, buf_size, "<function TODO>"); } break;
  case Type_nil: { written += snprintf(buf, buf_size, "nil"); } break;
  case Type_never: { Todo(); } break;
  case Type_slice: { Todo(); } break;
  case Type_array: { Todo(); } break;
  case Type_distinct: { Todo(); } break;
  case Type_sequence: { Todo(); } break;
  case Type_type: { written += snprintf(buf, buf_size, "<a type>"); } break;
  }

  return written;
}
