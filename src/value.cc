#include "blu.hh"

u32 ValueStore::value_to_string(TypeInterner *types, ValueIndex idx, char *buf, u32 buf_size) {
  u32 buf_size_start = buf_size;

  Value *v = get(idx);

#define Update(Code)                                                                               \
  do {                                                                                             \
    u32 _offset = (Code);                                                                          \
    _offset = min(_offset, buf_size); \
    buf      += _offset;                                                                         \
    buf_size -= _offset;                                                                         \
  } while (false)

#if 1
  Update(snprintf(buf, buf_size, "("));
  Update(types->type_to_string(v->type, buf, buf_size));
  Update(snprintf(buf, buf_size, ")"));
#endif

  auto t = types->get(v->type);

  switch (t->kind) {
  case Type_integer:
  case Type_literal_int: {
    i64 i     = *cast<i64 *>(v->data);
    Update(snprintf(buf, buf_size, "%lld", i));
  } break;
  case Type_boolean: {
    u8 b     = *cast<u8 *>(v->data);
    Update(snprintf(buf, buf_size, "%s", (b == 1) ? "true" : "false"));
  } break;
  case Type_literal_function: {
    Todo();
  } break;
  case Type_function: {
    Update(snprintf(buf, buf_size, "<function TODO>"));
  } break;
  case Type_nil: {
    Update(snprintf(buf, buf_size, "nil"));
  } break;
  case Type_never: {
    Todo();
  } break;
  case Type_slice: {
    Todo();
  } break;
  case Type_array: {
    Todo();
  } break;
  case Type_distinct: {
    Todo();
  } break;
  case Type_sequence: {
    Todo();
  } break;
  case Type_type: {
    Update(snprintf(buf, buf_size, "<a type>"));
  } break;
  }

  return buf_size_start - buf_size;
}
