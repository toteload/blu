#include "value.h"

void values_init(ValueStore *values, ValueStoreOptions *options) {
  arena_init(&values->arena, &(ArenaOptions){
    .reserve_size        = MiB(64),
    .initial_commit_size = KiB(64),
  });

  values->payload_allocator = options->payload_allocator;
}
