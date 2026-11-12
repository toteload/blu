#include "types.h"

#define XXH_INLINE_ALL
#include "xxhash.h"

#include <stdarg.h>

internal b32 cmp_type(void *context, Type *a, Type *b);
internal u32 hash_type(void *context, Type *x);

internal Type *intern_type(Arena *arena, Type *x) {
  u32 size = type_intern_byte_size(x);
  Type *intern = arena_push(arena, size, Align_of(Type));
  memcpy(intern, x, size);
  return intern;
}

#define INTERNER_NAME TypeInternerAux
#define INTERNER_TYPE TypePtr
#define INTERNER_INDEX_TYPE u32
#define INTERNER_FUNCTION_PREFIX aux
#define INTERNER_HASH_FN hash_type
#define INTERNER_COMPARE_FN cmp_type
#define INTERNER_COPY_FN intern_type
#define INTERNER_LINKAGE internal
#define INTERNER_RESERVE_ZERO_INDEX
#define INTERNER_OUTPUT_DEFINITIONS
#include "interner.h"

void types_init(TypeInterner *types, InternerOptions *options) {
  aux_init(&types->_interner, options);

  // clang-format off
  aux_add(&types->_interner, &(Type){.kind = Type_stub});
  aux_add(&types->_interner, &(Type){.kind = Type_type});
  aux_add(&types->_interner, &(Type){.kind = Type_nil});
  aux_add(&types->_interner, &(Type){.kind = Type_bool, .data.integer = { .signedness = Unsigned, .bitwidth = 8}});
  aux_add(&types->_interner, &(Type){.kind = Type_never});
  aux_add(&types->_interner, &(Type){.kind = Type_comptime_int});
  aux_add(&types->_interner, &(Type){.kind = Type_integer, .data.integer = {.signedness = Unsigned, .bitwidth = 8}});
  aux_add(&types->_interner, &(Type){.kind = Type_integer, .data.integer = {.signedness = Unsigned, .bitwidth = 16}});
  aux_add(&types->_interner, &(Type){.kind = Type_integer, .data.integer = {.signedness = Unsigned, .bitwidth = 32}});
  aux_add(&types->_interner, &(Type){.kind = Type_integer, .data.integer = {.signedness = Unsigned, .bitwidth = 64}});
  aux_add(&types->_interner, &(Type){.kind = Type_integer, .data.integer = {.is_uisize = True, .signedness = Unsigned, .bitwidth = 64}});
  aux_add(&types->_interner, &(Type){.kind = Type_integer, .data.integer = {.signedness = Signed, .bitwidth = 8}});
  aux_add(&types->_interner, &(Type){.kind = Type_integer, .data.integer = {.signedness = Signed, .bitwidth = 16}});
  aux_add(&types->_interner, &(Type){.kind = Type_integer, .data.integer = {.signedness = Signed, .bitwidth = 32}});
  aux_add(&types->_interner, &(Type){.kind = Type_integer, .data.integer = {.signedness = Signed, .bitwidth = 64}});
  aux_add(&types->_interner, &(Type){.kind = Type_integer, .data.integer = {.is_uisize = True, .signedness = Signed, .bitwidth = 64}});
  // clang-format on
}

void types_deinit(TypeInterner *types) {
  Todo();
}

TypeKey types_intern(TypeInterner *types, Type *t) {
  return (TypeKey){ aux_add(&types->_interner, t) };
}

Type *types_get(TypeInterner *types, TypeKey key) {
  return aux_get(&types->_interner, key.x);
}

internal b32 cmp_type(void *context, Type *a, Type *b) {
  Unused(context);

  if (a->kind != b->kind) {
    return False;
  }

  switch (Cast(TypeKind, a->kind)) {
  case Type_stub:
  case Type_nil:
  case Type_never:
  case Type_bool:
  case Type_type:
  case Type_comptime_int:
    return True;
  case Type_slice:
    return key_eq(a->data.slice.base_type, b->data.slice.base_type);
  case Type_pointer:
    return key_eq(a->data.pointer.base_type, b->data.pointer.base_type);
  case Type_function: {
    if (!key_eq(a->data.function.return_type, b->data.function.return_type)) {
      return False;
    }

    if (a->data.function.param_count != b->data.function.param_count) {
      return False;
    }

    for (u32 i = 0; i < a->data.function.param_count; i++) {
      if (!key_eq(a->data.function.param_types[i], b->data.function.param_types[i])) {
        return False;
      }
    }

    return True;
  }
  case Type_integer:
    return a->data.integer.signedness == b->data.integer.signedness &&
           a->data.integer.bitwidth == b->data.integer.bitwidth &&
           a->data.integer.is_uisize == b->data.integer.is_uisize;
  case Type_array: {
    return a->data.array.size == b->data.array.size &&
           key_eq(a->data.array.base_type, b->data.array.base_type);
  }
  }

  return False;
}

internal u32 push_type_data(Arena *arena, Type *x) {
#define Push_data(y)                                                                               \
  {                                                                                                \
    void *_p = arena_push(arena, sizeof(y), 1);                                                    \
    memcpy(_p, &(y), sizeof(y));                                                                   \
    size += sizeof(y);                                                                             \
  }

  u32 size = 0;

  Push_data(x->kind);

  switch (Cast(TypeKind, x->kind)) {
  case Type_stub:
  case Type_nil:
  case Type_never:
  case Type_bool:
  case Type_type:
  case Type_comptime_int:
    break;
  case Type_integer: {
    Push_data(x->data.integer.is_uisize);
    Push_data(x->data.integer.signedness);
    Push_data(x->data.integer.bitwidth);
  } break;
  case Type_slice: {
    Push_data(x->data.slice.base_type);
  } break;
  case Type_pointer: {
    Push_data(x->data.pointer.base_type);
  } break;
  case Type_function: {
    Push_data(x->data.function.return_type);
    Push_data(x->data.function.param_count);
    for (u32 i = 0; i < x->data.function.param_count; i++) {
      Push_data(x->data.function.param_types[i]);
    }
  } break;
  case Type_array: {
    Push_data(x->data.array.size);
    Push_data(x->data.array.base_type);
  } break;
  }

  return size;

#undef Push_data
}

internal u32 hash_type(void *context, Type *x) {
  Arena *scratch = context;
  ArenaSnapshot snapshot = arena_scope_begin(scratch);

  void *data = scratch->at;
  u32 size = push_type_data(scratch, x);
  u32 hash = XXH32(data, size, 0);

  arena_scope_end(scratch, snapshot);

  return hash;
}

u32 type_intern_byte_size(Type *type) {
  switch (Cast(TypeKind, type->kind)) {
  case Type_stub:
  case Type_comptime_int:
  case Type_integer:
  case Type_bool:
  case Type_nil:
  case Type_never:
  case Type_type:
  case Type_slice:
  case Type_pointer:
  case Type_array:
    return sizeof(Type);
  case Type_function:
    return sizeof(Type) + type->data.function.param_count * sizeof(TypeKey);
  }

  Unreachable();

  return 0;
}

TypeSizeInfo types_size_info(TypeInterner *types, Type *type) {
  // ASSUME: pointers are 8 bytes.

  switch (Cast(TypeKind, type->kind)) {
  case Type_comptime_int: Unreachable();
  case Type_stub: Unreachable();
  case Type_bool:
    return (TypeSizeInfo){.size = 1, .align = 1, .stride = 1};
  case Type_nil:
    return (TypeSizeInfo){.size = 0, .align = 0, .stride = 0};
  case Type_never:
    return (TypeSizeInfo){.size = 0, .align = 0, .stride = 0};
  case Type_type:
    return (TypeSizeInfo){.size = 4, .align = 4, .stride = 4};
  case Type_pointer:
    return (TypeSizeInfo){.size = 8, .align = 8, .stride = 8};
  case Type_integer: {
    // ASSUME: the bitwidth of integers is always a multiple of 8.
    u32 size = type->data.integer.bitwidth / 8;
    return (TypeSizeInfo){.size = size, .align = size, .stride = size};
  }
  case Type_slice: {
    // A slice is stored as an 8-byte pointer and an 8-byte length.
    return (TypeSizeInfo){.size = 16, .align = 8, .stride = 16};
  }
  case Type_array: {
    TypeSizeInfo size_info = types_size_info_by_key(types, type->data.array.base_type);
    u32 size = size_info.stride * type->data.array.size;
    return (TypeSizeInfo){
      .size = size,
      .align = size_info.align,
      .stride = size,
    };
  }
  case Type_function: {
    // A variable holding a function is stored as the ResidualFunctionKey of the function.
    return (TypeSizeInfo){.size = 4, .align = 4, .stride = 4};
  }
  }

  Unreachable();

  return (TypeSizeInfo){0};
}

TypeSizeInfo types_size_info_by_key(TypeInterner *types, TypeKey idx) {
  Type *type = types_get(types, idx);
  return types_size_info(types, type);
}

// assumes that
// - from is a comptime type
// - to and from are not the same
u32 classify_comptime_coercion(TypeInterner *types, TypeKey to, TypeKey from) {
  Type *type_to = types_get(types, to);
  Type *type_from = types_get(types, from);

  if (type_from->kind == Type_comptime_int) {
    if (type_to->kind == Type_integer) {
      return Coercion_comptime;
    }

    return Coercion_invalid;
  }

  if (classify_type_coercion(types, to, from) == Coercion_invalid) {
    return Coercion_invalid;
  }

  return Coercion_comptime;
}

u32 classify_type_coercion(TypeInterner *types, TypeKey to, TypeKey from) {
  if (key_eq(to, from)) {
    return Coercion_identical_types;
  }

  Type *type_to = types_get(types, to);
  Type *type_from = types_get(types, from);

  if (type_from->kind == Type_integer && type_to->kind == Type_integer) {
    if (type_from->data.integer.signedness == type_to->data.integer.signedness &&
        type_from->data.integer.bitwidth <= type_to->data.integer.bitwidth) {
      return Coercion_int_widen;
    }

    return Coercion_invalid;
  }

  if (type_from->kind == Type_bool && type_to->kind == Type_integer) {
    return Coercion_bool_to_int;
  }

  if (type_from->kind == Type_array && type_to->kind == Type_slice) {
    if (key_eq(type_from->data.array.base_type, type_to->data.slice.base_type)) {
      return Coercion_array_to_slice;
    }

    return classify_comptime_coercion(types, type_to->data.slice.base_type, type_from->data.array.base_type);
  }

  if (type_from->kind == Type_array && type_to->kind == Type_array) {
    if (type_from->data.array.size != type_to->data.array.size) {
      return Coercion_invalid;
    }

    return classify_comptime_coercion(types, type_to->data.array.base_type, type_from->data.array.base_type);
  }

  if (type_from->kind == Type_slice && type_to->kind == Type_slice) {
    if (!is_type_comptime(types, type_from->data.slice.base_type)) {
      return Coercion_invalid;
    }

    return classify_comptime_coercion(types, type_to->data.slice.base_type, type_from->data.slice.base_type);
  }

  if (is_type_comptime(types, from)) {
    return classify_comptime_coercion(types, to, from);
  }

  return Coercion_invalid;
}

internal b32 is_type_int_like(TypeKey t) {
  return (t.x >= TYPE_COMPTIME_INT.x) && (t.x <= TYPE_ISIZE.x);
}

u32 classify_type_cast(TypeInterner *types, TypeKey to, TypeKey from) {
  if (is_type_comptime(types, to)) {
    return CAST_INVALID;
  }

  u32 coercion = classify_type_coercion(types, to, from);
  if (coercion != Coercion_invalid) {
    return coercion;
  }

  if (is_type_int_like(to) && is_type_int_like(from)) {
    return CAST_INT_RANGE_CHECK;
  }

  return CAST_INVALID;
}

TypeKey types_add_pointer(TypeInterner *types, TypeKey base_type) {
  return types_intern(types, &(Type){.kind = Type_pointer, .data.pointer = {.base_type = base_type}});
}

b32 check_type_for_op_bitshift(TypeKey lhs, TypeKey rhs) {
  return is_type_int_like(lhs) && is_type_int_like(rhs);
}

b32 check_type_for_op_bitwise(TypeKey t) {
  return is_type_int_like(t);
}

b32 check_type_for_op_arithmetic(TypeKey t) {
  return is_type_int_like(t);
}

b32 check_type_for_op_cmp(TypeKey t) {
  return is_type_int_like(t) || key_eq(t, TYPE_BOOL);
}

b32 check_type_for_op_negate(TypeKey t) {
  return is_type_int_like(t);
}

b8 is_type_comptime(TypeInterner *types, TypeKey t) {
  Type *p = types_get(types, t);
  switch (Cast(TypeKind, p->kind)) {
  case Type_comptime_int: return True;
  case Type_array: return is_type_comptime(types, p->data.array.base_type);
  case Type_slice: return is_type_comptime(types, p->data.slice.base_type);
  default: return False;
  }
}

internal String arena_printf(Arena *arena, char const *format, ...) {
  va_list args;
  va_start(args, format);
  i32 res = vsnprintf(Null, 0, format, args);
  va_end(args);

  Assert(res >= 0);

  u32 len = Cast(u32, res);

  u8 *s = arena_push_array(u8, arena, len + 1);

  va_start(args, format);
  vsnprintf(Cast(char *, s), len + 1, format, args);
  va_end(args);

  arena->at = ptr_offseti(arena->at, -1);

  return (String){.str = s, .len = len};
}

String write_type(Arena *arena, TypeInterner *types, TypeKey type) {
  if (key_is_nil(type)) {
    return arena_copy_string(arena, string_lit("?"));
  }

  Type *t = types_get(types, type);
  switch (Cast(TypeKind, t->kind)) {
  case Type_comptime_int: return arena_copy_string(arena, string_lit("comptime_int"));
  case Type_bool: return arena_copy_string(arena, string_lit("bool"));
  case Type_stub: return arena_copy_string(arena, string_lit("stub"));
  case Type_nil: return arena_copy_string(arena, string_lit("nil"));
  case Type_never: return arena_copy_string(arena, string_lit("never"));
  case Type_type: return arena_copy_string(arena, string_lit("type"));
  case Type_integer: {
    if (t->data.integer.is_uisize) {
      return arena_printf(
        arena,
        "%csize",
        (t->data.integer.signedness == Signed) ? 'i' : 'u');
    }

    return arena_printf(
      arena,
      "%c%u",
      (t->data.integer.signedness == Signed) ? 'i' : 'u',
      t->data.integer.bitwidth);
  } break;
  case Type_array: {
    String s = arena_printf(arena, "[%u]", t->data.array.size);
    String a = write_type(arena, types, t->data.array.base_type);
    return (String){.str = s.str, .len = s.len + a.len};
  } break;
  case Type_function: {
    String s = arena_printf(arena, "(");
    u32 len = s.len;
    for (u32 i = 0; i < t->data.function.param_count; i++) {
      if (i != 0) {
        len += arena_printf(arena, ", ").len;
      }
      len += write_type(arena, types, t->data.function.param_types[i]).len;
    }
    len += arena_printf(arena, ") ").len;
    len += write_type(arena, types, t->data.function.return_type).len;
    return (String){.str = s.str, .len = len};
  } break;
  case Type_slice: {
    String s = arena_printf(arena, "[]", t->data.array.size);
    String a = write_type(arena, types, t->data.array.base_type);
    return (String){.str = s.str, .len = s.len + a.len};
  } break;
  case Type_pointer: {
    String s = arena_printf(arena, "*", t->data.array.size);
    String a = write_type(arena, types, t->data.array.base_type);
    return (String){.str = s.str, .len = s.len + a.len};
  } break;
  }
}
