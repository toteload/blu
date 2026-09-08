#include "value_interner.h"

typedef struct {
  TypeInterner *types;
} Context;

internal b8 cmp_value(void *context, Value *a, Value *b) {
  Context *ctx = context;

  if (a->type != b->type) {
    return False;
  }

  Type t = types_get(ctx->types, a->type);

  switch (Cast(TypeKind, t->kind)) {
  case Type_never:
  case Type_nil: return True;
  case Type_pointer: {
    ValuePointer *pa = &a->data;
    ValuePointer *bp = &b->data;
    return key_eq(pa->val, pb->val) && pa->offset == pb->offset;
  } break;
  }

  Todo();
}

internal u32 hash_value(void *context, Value *x);

internal Value *intern_value(Arena *arena, Type *x) {
  u32 size = type_intern_byte_size(x);
  Type *intern = arena_push(arena, size, Align_of(Type));
  memcpy(intern, x, size);
  return intern;
}


