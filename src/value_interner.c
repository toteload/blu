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

internal void intern_type(ValueInterner *values, Arena *scratch, TypeInterner *types, TypeKey type) {
  u32 size = sizeof(Value) + sizeof(TypeKey);
  Value *v = arena_push(scratch, size, Align_of(Value));
  v->type = types->t_type;
  v->size = size;
  TypeKey *t = Cast(TypeKey*, &v->data);
  *t = type;
  aux_add(&values->aux, v);
}

void values2_init(ValueInterner *values, ValueInternerInitOptions *options) {
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

  intern_type(values, scratch, types, types->t_type);
  intern_type(values, scratch, types, types->t_comptime_int);
  intern_type(values, scratch, types, types->t_never);
  intern_type(values, scratch, types, types->t_bool);
  intern_type(values, scratch, types, types->t_i8);
  intern_type(values, scratch, types, types->t_i16);
  intern_type(values, scratch, types, types->t_i32);
  intern_type(values, scratch, types, types->t_i64);
  intern_type(values, scratch, types, types->t_u8);
  intern_type(values, scratch, types, types->t_u16);
  intern_type(values, scratch, types, types->t_u32);
  intern_type(values, scratch, types, types->t_u64);
  intern_type(values, scratch, types, types->t_usize);
  intern_type(values, scratch, types, types->t_isize);

  {
    TypeKey t = types->t_bool;
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
    TypeKey t = types->t_bool;
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
    TypeKey t = types->t_comptime_int;
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

void values2_deinit(ValueInterner *values) {
  aux_deinit(&values->aux);
}

ValueKey values2_intern(ValueInterner *values, Value *v) {
  return aux_add(&values->aux, v);
}

Value *values2_get(ValueInterner *values, ValueKey key) {
  return aux_get(&values->aux, key);
}
