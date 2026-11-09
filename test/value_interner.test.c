#include "toteload.h"
#include "blu.h"
#include "types.h"
#include "value_interner.h"
#include "test.h"

#include <stdlib.h>

internal void *test_alloc_fn(void *ctx, void *p, usize old_byte_size, usize new_byte_size, u32 align) {
  Unused(ctx, old_byte_size, align);

  if (!is_null(p) && new_byte_size == 0) {
    free(p);
    return Null;
  }

  return realloc(p, new_byte_size);
}

typedef struct {
  Arena storage;
  Arena scratch;
  TypeInterner types;
  ValueInterner values;
} Fixture;

internal void fixture_init(Fixture *f) {
  arena_init(&f->storage, &(ArenaOptions){ .reserve_size = MiB(1), .initial_commit_size = KiB(64) });
  arena_init(&f->scratch, &(ArenaOptions){ .reserve_size = MiB(1), .initial_commit_size = KiB(64) });

  types_init(&f->types, &(InternerOptions){
    .arena            = &f->storage,
    .map_allocator    = (Allocator){ .fn = test_alloc_fn },
    .map_initial_size = 32,
    .context          = &f->scratch,
  });

  values_init(&f->values, &(ValueInternerInitOptions){
    .storage       = &f->storage,
    .map_allocator = (Allocator){ .fn = test_alloc_fn },
    .scratch       = &f->scratch,
    .types         = &f->types,
  });
}

internal void fixture_deinit(Fixture *f) {
  values_deinit(&f->values);
  arena_deinit(&f->scratch);
  arena_deinit(&f->storage);
}

internal Value *make_value(Arena *arena, TypeKey type, void const *data, u32 data_size) {
  u32 size = Cast(u32, sizeof(Value)) + data_size;
  Value *v = arena_push(arena, size, Align_of(Value));
  v->type = type;
  v->size = size;
  memcpy(v->data, data, data_size);
  return v;
}

internal ValueKey intern_type_value(Fixture *f, TypeKey t) {
  return values_intern(&f->values, make_value(&f->scratch, f->types.t_type, &t, sizeof(t)));
}

internal ValueKey intern_bool(Fixture *f, u8 b) {
  return values_intern(&f->values, make_value(&f->scratch, f->types.t_bool, &b, sizeof(b)));
}

internal void test_value_interner_preinterned(TestResult *test, void *user) {
  Unused(user);

  Fixture f;
  fixture_init(&f);

  Test_assert_eq(intern_type_value(&f, f.types.t_type).x, VAL_TYPE_TYPE);
  Test_assert_eq(intern_type_value(&f, f.types.t_comptime_int).x, VAL_TYPE_COMPTIME_INT);
  Test_assert_eq(intern_type_value(&f, f.types.t_never).x, VAL_TYPE_NEVER);
  Test_assert_eq(intern_type_value(&f, f.types.t_bool).x, VAL_TYPE_BOOL);
  Test_assert_eq(intern_type_value(&f, f.types.t_i8).x, VAL_TYPE_I8);
  Test_assert_eq(intern_type_value(&f, f.types.t_i16).x, VAL_TYPE_I16);
  Test_assert_eq(intern_type_value(&f, f.types.t_i32).x, VAL_TYPE_I32);
  Test_assert_eq(intern_type_value(&f, f.types.t_i64).x, VAL_TYPE_I64);
  Test_assert_eq(intern_type_value(&f, f.types.t_u8).x, VAL_TYPE_U8);
  Test_assert_eq(intern_type_value(&f, f.types.t_u16).x, VAL_TYPE_U16);
  Test_assert_eq(intern_type_value(&f, f.types.t_u32).x, VAL_TYPE_U32);
  Test_assert_eq(intern_type_value(&f, f.types.t_u64).x, VAL_TYPE_U64);
  Test_assert_eq(intern_type_value(&f, f.types.t_usize).x, VAL_TYPE_USIZE);
  Test_assert_eq(intern_type_value(&f, f.types.t_isize).x, VAL_TYPE_ISIZE);

  Test_assert_eq(intern_bool(&f, 1).x, VAL_BOOL_TRUE);
  Test_assert_eq(intern_bool(&f, 0).x, VAL_BOOL_FALSE);

  Value *one = values_get(&f.values, (ValueKey){ VAL_COMPTIME_INT_1 });
  Test_assert(key_eq(one->type, f.types.t_comptime_int));
  ComptimeInt x;
  memcpy(&x, one->data, sizeof(x));
  Test_assert_eq(x, 1);

  fixture_deinit(&f);
}

internal void test_value_interner_dedup(TestResult *test, void *user) {
  Unused(user);

  Fixture f;
  fixture_init(&f);

  TypeKey ptr_u8 = types_add_pointer(&f.types, f.types.t_u8);

  ValueKey a = intern_type_value(&f, ptr_u8);
  ValueKey b = intern_type_value(&f, ptr_u8);
  Test_assert(key_eq(a, b));
  Test_assert(a.x > VAL_COMPTIME_INT_1);

  ValueKey c = intern_type_value(&f, types_add_pointer(&f.types, f.types.t_u16));
  Test_assert(!key_eq(a, c));

  fixture_deinit(&f);
}

internal void test_value_interner_pointer(TestResult *test, void *user) {
  Unused(user);

  Fixture f;
  fixture_init(&f);

  TypeKey ptr_bool = types_add_pointer(&f.types, f.types.t_bool);

  ValuePointer p0 = { .val = { VAL_BOOL_TRUE }, .offset = 0 };
  ValuePointer p1 = { .val = { VAL_BOOL_TRUE }, .offset = 1 };
  ValuePointer p2 = { .val = { VAL_BOOL_FALSE }, .offset = 0 };

  ValueKey k0 = values_intern(&f.values, make_value(&f.scratch, ptr_bool, &p0, sizeof(p0)));
  ValueKey k1 = values_intern(&f.values, make_value(&f.scratch, ptr_bool, &p1, sizeof(p1)));
  ValueKey k2 = values_intern(&f.values, make_value(&f.scratch, ptr_bool, &p2, sizeof(p2)));
  ValueKey k0_again = values_intern(&f.values, make_value(&f.scratch, ptr_bool, &p0, sizeof(p0)));

  Test_assert(!key_eq(k0, k1));
  Test_assert(!key_eq(k0, k2));
  Test_assert(key_eq(k0, k0_again));

  fixture_deinit(&f);
}

internal void test_value_interner_copies(TestResult *test, void *user) {
  Unused(user);

  Fixture f;
  fixture_init(&f);

  TypeKey ptr_u8 = types_add_pointer(&f.types, f.types.t_u8);
  Value *v = make_value(&f.scratch, f.types.t_type, &ptr_u8, sizeof(ptr_u8));
  ValueKey k = values_intern(&f.values, v);

  Value *stored = values_get(&f.values, k);
  Test_assert(stored != v);
  Test_assert_eq(stored->size, v->size);

  TypeKey other = f.types.t_u64;
  memcpy(v->data, &other, sizeof(other));

  TypeKey t;
  memcpy(&t, stored->data, sizeof(t));
  Test_assert(key_eq(t, ptr_u8));

  fixture_deinit(&f);
}

void register_value_interner_tests(TestRunner *runner) {
  test_runner_register_test(runner, string_lit("test_value_interner_preinterned"), test_value_interner_preinterned, Null);
  test_runner_register_test(runner, string_lit("test_value_interner_dedup"), test_value_interner_dedup, Null);
  test_runner_register_test(runner, string_lit("test_value_interner_pointer"), test_value_interner_pointer, Null);
  test_runner_register_test(runner, string_lit("test_value_interner_copies"), test_value_interner_copies, Null);
}
