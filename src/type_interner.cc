#include "blu.hh"

#define XXH_INLINE_ALL
#include "xxhash.h"

b32 type_eq(void *context, Type a, Type b) {
  if (a.kind != b.kind) {
    return false;
  }

  switch (a.kind) {
  case Boolean: return true;
  case Function: {
      if (a.function.return_type != b.function.return_type) {
        return false;
      }

    Type *pa = a.function.params;
    Type *pb = b.function.params;

    while (pa == pb && pa != nullptr) {
      pa = pa->next;
      pb = pb->next;
    }

    return pa == pb;
  }
  case Integer:
      return a.integer.signedness == b.integer.signedness && a.integer.bitwidth == b.integer.bitwidth;
  }
}

u32 push_type_data(Arena *arena, Type *x) {
  void *kind = arena->raw_alloc(sizeof(TypeKind), 1);
  memcpy(kind, &x->kind, sizeof(TypeKind));

  u32 size = sizeof(TypeKind);

  switch (x->kind) {
  case Boolean: break;
  case Integer: {
    void *signedness = arena->raw_alloc(sizeof(Signedness), 1);
    memcpy(signedness, &x->integer.signedness, sizeof(Signedness));
    size += sizeof(Signedness);

    void *bitwidth = arena->raw_alloc(sizeof(u16), 1);
    memcpy(bitwidth, &x->integer.bitwidth, sizeof(u16));
    size += sizeof(u16);
  } break;
  case Function: {
    size += push_type_data(arena, x->function.return_type);

    u16 param_count = 0;
    Type *param = x->function.params;
    while (param) {
      param = param->next;
      param_count += 1;
    }

    void *count = arena->raw_alloc(sizeof(param_count), 1);
    memcpy(count, &param_count, sizeof(param_count));
    size += sizeof(param_count);

    param = x->function.params;
    while (param) {
      size += push_type_data(arena, param);
    }
  } break;
  default: Unreachable();
  }

  return size;
}

u32 type_hash(void *context, Type x) {
  Arena *arena = cast<Arena*>(context);
  auto snapshot = arena->take_snapshot();

  void *data = snapshot.at;
  u32 size = push_type_data(arena, &x);
  u32 hash = XXH32(data, size, 0);

  arena->restore(snapshot);

  return hash;
}

void TypeInterner::init(Arena *arena, Allocator map_allocator, Allocator pool_allocator) {
  map.init(map_allocator, arena);
  pool.init(pool_allocator);
}

void TypeInterner::deinit() {}

Type *TypeInterner::add(Type type) {
  b32 was_occupied;
  auto bucket = map.insert_key_and_get_bucket(type, &was_occupied);
  if (was_occupied) {
    return bucket->val;
  }

  Type *t = pool.alloc();
  *t = type;

  bucket->val = t;

  return t;
}

