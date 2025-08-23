#include "blu.hh"

// Should this Slice<char> be a Str?
// Should Str be a subtype of Slice?
u32 Type::write_string(Slice<char> out) {
  switch (kind) {
  case Type_Integer:
    return snprintf(out.data, out.len(), "%s%d", integer.signedness == Signed ? "i" : "u", integer.bitwidth);
  case Type_IntegerConstant:
    return snprintf(out.data, out.len(), "int-constant");
  case Type_Boolean:
    return snprintf(out.data, out.len(), "bool");
  case Type_Void:
    return cast<u32>(snprintf(out.data, out.len(), "void"));
  case Type_Never:
    return cast<u32>(snprintf(out.data, out.len(), "never"));
  case Type_slice: {
    u32 offset = 0;
    offset += cast<u32>(snprintf(out.data, out.len(), "[]"));
    offset += slice.base_type->write_string(out.sub(offset, out.len()));
    return offset;
  }
    case Type_distinct: {
      u32 offset = 0;
      offset += cast<u32>(snprintf(out.data, out.len(), "distinct(%d) ", distinct.uid));
      offset += distinct.base->write_string(out.sub(offset, out.len()));
      return offset;
    }
  }
  Todo();
  return 0;
}
