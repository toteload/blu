#include "toteload.h"
#include <stdio.h>

// Same parameters as the NodeIndexList instantiation in src/ast.h / src/parse.c.
#define SEGMENTLIST_NAME            IntList
#define SEGMENTLIST_FUNCTION_PREFIX list
#define SEGMENTLIST_TYPE            i32
#define SEGMENTLIST_MIN_SIZE_LOG2   3
#define SEGMENTLIST_SEGMENT_COUNT   24
#define SEGMENTLIST_OUTPUT_TYPES
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

#define Item_count 1000

i32 *item_ptrs[Item_count];

i32 main() {
  Arena arena;
  arena_init(&arena, &(ArenaOptions){ .reserve_size = MiB(64), });

  IntList list = {};

  // A fresh list is empty.
  Assert(list.len == 0);
  Assert(list.segment_count == 0);

  // Pushed items are readable back through ptr_at_unchecked.
  for (usize i = 0; i < Item_count; i++) {
    i32 value = Cast(i32, i * 3 + 1);
    list_append(&list, &arena, value);

    Assert(list.len == i + 1);

    i32 *p = list_ptr_at_unchecked(&list, i);
    Assert(!is_null(p));
    Assert(*p == value);

    item_ptrs[i] = p;
  }

  Assert(list.len == Item_count);
  Assert(list_cap(&list) >= Item_count);

  // Items never move: pointers taken at push time stay valid.
  // Re-checking every value also catches any two indices aliasing the same slot.
  for (usize i = 0; i < Item_count; i++) {
    Assert(list_ptr_at_unchecked(&list, i) == item_ptrs[i]);
    Assert(*item_ptrs[i] == Cast(i32, i * 3 + 1));
  }

  // Writes through push stick.
  i32 *p = list_push(&list, &arena);
  *p = 12345;
  Assert(list.len == Item_count + 1);
  Assert(*list_ptr_at_unchecked(&list, Item_count) == 12345);

  // Two lists growing interleaved from the same arena stay independent.
  IntList a = {};
  IntList b = {};

  for (usize i = 0; i < 100; i++) {
    list_append(&a, &arena, Cast(i32, i));
    list_append(&b, &arena, Cast(i32, 1000 + i));
  }

  for (usize i = 0; i < 100; i++) {
    Assert(*list_ptr_at_unchecked(&a, i) == Cast(i32, i));
    Assert(*list_ptr_at_unchecked(&b, i) == Cast(i32, 1000 + i));
  }

  printf("ok\n");

  return 0;
}
