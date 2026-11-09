#include "value_interner.h"
#include "types.h"

#define XXH_INLINE_ALL
#include "xxhash.h"

internal b8 cmp_value(void *context, Value *a, Value *b) {
  ValueInternerContext *ctx = context;

  if (a->size != b->size) {
    return False;
  }

  return memcmp(a, b, a->size) == 0;
}

internal u32 hash_value(void *context, Value *x) {
  return XXH32(x, x->size, 0);
}

internal Value *copy_value(Arena *arena, Value *x) {
  Value *c = arena_push(arena, x->size, Align_of(Value));
  memcpy(c, x, x->size);
  return c;
}

#define INTERNER_NAME            AuxValueInterner
#define INTERNER_TYPE            ValuePtr
#define INTERNER_INDEX_TYPE      ValueKey
#define INTERNER_FUNCTION_PREFIX aux
#define INTERNER_HASH_FN         hash_value
#define INTERNER_COMPARE_FN      cmp_value
#define INTERNER_COPY_FN         copy_value
#define INTERNER_LINKAGE internal
#define INTERNER_RESERVE_ZERO_INDEX
#define INTERNER_INDEX_IS_KEY
#define INTERNER_OUTPUT_DECLARATIONS
#define INTERNER_OUTPUT_DEFINITIONS
#include "interner.h"

internal void intern_type(ValueInterner *values, Arena *scratch, TypeKey type) {
  u32 size = sizeof(Value) + sizeof(TypeKey);
  Value *v = arena_push(scratch, size, Align_of(Value));
  v->type = TYPE_TYPE;
  v->size = size;
  TypeKey *t = Cast(TypeKey*, &v->data);
  *t = type;
  aux_add(&values->aux, v);
}

void values_init(ValueInterner *values, ValueInternerInitOptions *options) {
  TypeInterner *types = options->types;
  Arena *scratch = options->scratch;

  values->context = (ValueInternerContext){
    .types = options->types,
    .scratch = options->scratch,
  };

  aux_init(&values->aux, &(InternerOptions){
    .arena = options->storage,
    .map_allocator = options->map_allocator,
    .context = &values->context,
  });

  ArenaSnapshot snap = arena_scope_begin(scratch);

  intern_type(values, scratch, TYPE_TYPE);
  intern_type(values, scratch, TYPE_COMPTIME_INT);
  intern_type(values, scratch, TYPE_NEVER);
  intern_type(values, scratch, TYPE_BOOL);
  intern_type(values, scratch, TYPE_I8);
  intern_type(values, scratch, TYPE_I16);
  intern_type(values, scratch, TYPE_I32);
  intern_type(values, scratch, TYPE_I64);
  intern_type(values, scratch, TYPE_U8);
  intern_type(values, scratch, TYPE_U16);
  intern_type(values, scratch, TYPE_U32);
  intern_type(values, scratch, TYPE_U64);
  intern_type(values, scratch, TYPE_USIZE);
  intern_type(values, scratch, TYPE_ISIZE);
  intern_type(values, scratch, TYPE_NIL);

  {
    TypeKey t = TYPE_BOOL;
    TypeSizeInfo size_info = types_size_info_by_key(types, t);
    u32 size = sizeof(Value) + size_info.size;
    Value *v = arena_push(scratch, size, Align_of(Value));
    v->type = t;
    v->size = size;
    u8 *x = Cast(u8*, &v->data);
    *x = 1;
    aux_add(&values->aux, v);
  }

  {
    TypeKey t = TYPE_BOOL;
    TypeSizeInfo size_info = types_size_info_by_key(types, t);
    u32 size = sizeof(Value) + size_info.size;
    Value *v = arena_push(scratch, size, Align_of(Value));
    v->type = t;
    v->size = size;
    u8 *x = Cast(u8*, &v->data);
    *x = 0;
    aux_add(&values->aux, v);
  }

  {
    TypeKey t = TYPE_COMPTIME_INT;
    u32 size = sizeof(Value) + sizeof(ComptimeInt);
    Value *v = arena_push(scratch, size, Align_of(Value));
    v->type = t;
    v->size = size;
    ComptimeInt *x = Cast(ComptimeInt*, &v->data);
    *x = 1;
    aux_add(&values->aux, v);
  }

  arena_scope_end(scratch, snap);
}

void values_deinit(ValueInterner *values) {
  aux_deinit(&values->aux);
}

ValueKey values_intern(ValueInterner *values, Value *v) {
  return aux_add(&values->aux, v);
}

ValueKey values_intern_data(ValueInterner *values, TypeKey type, void const *data, u32 data_size) {
  Arena *scratch = values->context.scratch;
  ArenaSnapshot snap = arena_scope_begin(scratch);

  u32 size = Cast(u32, sizeof(Value)) + data_size;
  Value *v = arena_push(scratch, size, Align_of(Value));
  v->type = type;
  v->size = size;
  if (data_size > 0) {
    memcpy(v->data, data, data_size);
  }

  ValueKey res = aux_add(&values->aux, v);

  arena_scope_end(scratch, snap);

  return res;
}

Value *values_get(ValueInterner *values, ValueKey key) {
  return aux_get(&values->aux, key);
}
