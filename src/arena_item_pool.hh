#pragma once

#include "toteload.hh"

// This data structure was added to efficiently allocate items of a single type and manage them
// with keys that are smaller than pointers in size.
// Also getting a pointer to an item using its key should be cheap.
//
// This could use a better name. The fact that it uses an arena is an implementation detail and not
// important for what it is.

template<typename T>
struct ArenaItemPool {
  union Item {
    T item;
    Item *next;
  };

  Arena arena;
  u32 alloc_count = 0;
  u32 alloc_capacity = 0;
  Item *freelist = nullptr;

  void init(u32 reserve_count);
  void deinit();

  u32 reserve_index();
  void free_index(u32 index);

  Item *item_at_index(u32 index) const {
    return cast<Item*>(ptr_offset(arena.base, index * sizeof(T)));
  }

  T *get(u32 index);
};

template<typename T>
void ArenaItemPool<T>::init(u32 reserve_count) {
  arena.init(reserve_count * sizeof(T));
  alloc_count = 0;
  alloc_capacity = 0;
  freelist = nullptr;
}

template<typename T>
void ArenaItemPool<T>::deinit() {
  arena.deinit();
  memset(*this, 0, sizeof(*this));
}

template<typename T>
u32 ArenaItemPool<T>::reserve_index() {
  if (freelist == nullptr) {
    usize old_commit_size = arena.commit_size();
    usize commit_size = min<usize>(1, arena.commit_size() * 2);
    arena.commit(commit_size);

    Item *item = cast<Item*>(ptr_forward_align(ptr_offset(arena.base, old_commit_size), Align_of(T)));
    freelist = item;

    usize new_item_count = ptr_diff(arena.commit_end, item) / sizeof(T);
    for (usize i = 0; i < new_item_count - 1; i += 1) {
      Item *next = item + 1;
      item->next = next;
      item = next;
    }

    item->next = nullptr;
  }

  Item *item = freelist;
  freelist = freelist->next;

  return cast<u32>(ptr_diff(arena.base, item) / sizeof(T));
}

template<typename T>
void ArenaItemPool<T>::free_index(u32 index) {
  Item *item = item_at_index(index);
  item->next = freelist;
  freelist = item;
}

template<typename T>
T *ArenaItemPool<T>::get(u32 index) {
  Item *item = item_at_index(index);
  return cast<T*>(item);
}
