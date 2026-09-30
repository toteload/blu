#ifndef QUEUE_NAME
#error "QUEUE_NAME must be defined"
#endif

#ifndef QUEUE_TYPE
#error "QUEUE_TYPE must be defined"
#endif

#ifndef QUEUE_MIN_SIZE_LOG2
#error "QUEUE_MIN_SIZE_LOG2 must be defined"
#endif

#ifndef QUEUE_FUNCTION_PREFIX
#define QUEUE_FUNCTION_PREFIX QUEUE_NAME
#endif

#ifndef QUEUE_LINKAGE
#define QUEUE_LINKAGE
#endif

#include "toteload.h"

#ifdef QUEUE_OUTPUT_TYPES

typedef struct {
  usize head;
  usize len;
  usize cap;
  QUEUE_TYPE *data;
} QUEUE_NAME;

#undef QUEUE_OUTPUT_TYPES
#endif // QUEUE_OUTPUT_TYPES

#if defined(QUEUE_OUTPUT_DECLARATIONS) || defined(QUEUE_OUTPUT_DEFINITIONS)

QUEUE_LINKAGE void        Cat(QUEUE_FUNCTION_PREFIX, _deinit)(QUEUE_NAME *queue, Allocator allocator);
QUEUE_LINKAGE QUEUE_TYPE *Cat(QUEUE_FUNCTION_PREFIX, _push)(QUEUE_NAME *queue, Allocator allocator);
QUEUE_LINKAGE QUEUE_TYPE  Cat(QUEUE_FUNCTION_PREFIX, _pop)(QUEUE_NAME *queue);
QUEUE_LINKAGE void        Cat(QUEUE_FUNCTION_PREFIX, _append)(QUEUE_NAME *queue, Allocator allocator, QUEUE_TYPE item);
QUEUE_LINKAGE QUEUE_TYPE *Cat(QUEUE_FUNCTION_PREFIX, _ptr_at_unchecked)(QUEUE_NAME *queue, usize i);
QUEUE_LINKAGE QUEUE_TYPE  Cat(QUEUE_FUNCTION_PREFIX, _at_unchecked)(QUEUE_NAME *queue, usize i);

#undef QUEUE_OUTPUT_DECLARATIONS
#endif // QUEUE_OUTPUT_DECLARATIONS

#ifdef QUEUE_OUTPUT_DEFINITIONS
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"

QUEUE_LINKAGE
void Cat(QUEUE_FUNCTION_PREFIX, _deinit)(QUEUE_NAME *queue, Allocator allocator) {
  if (queue->cap > 0) {
    Free(allocator, queue->data, queue->cap * sizeof(QUEUE_TYPE));
  }

  *queue = (QUEUE_NAME){0};
}

internal void Cat(QUEUE_FUNCTION_PREFIX, __grow)(QUEUE_NAME *queue, Allocator allocator) {
  usize new_cap = queue->cap ? queue->cap * 2 : (usize)1 << QUEUE_MIN_SIZE_LOG2;
  QUEUE_TYPE *data = Alloc(allocator, new_cap * sizeof(QUEUE_TYPE), Align_of(QUEUE_TYPE));

  if (queue->cap > 0) {
    usize first = queue->cap - queue->head;
    if (first > queue->len) {
      first = queue->len;
    }

    memcpy(data, queue->data + queue->head, first * sizeof(QUEUE_TYPE));
    memcpy(data + first, queue->data, (queue->len - first) * sizeof(QUEUE_TYPE));

    Free(allocator, queue->data, queue->cap * sizeof(QUEUE_TYPE));
  }

  queue->data = data;
  queue->head = 0;
  queue->cap  = new_cap;
}

QUEUE_LINKAGE
QUEUE_TYPE *Cat(QUEUE_FUNCTION_PREFIX, _push)(QUEUE_NAME *queue, Allocator allocator) {
  if (queue->len == queue->cap) {
    Cat(QUEUE_FUNCTION_PREFIX, __grow)(queue, allocator);
  }

  QUEUE_TYPE *p = Cat(QUEUE_FUNCTION_PREFIX, _ptr_at_unchecked)(queue, queue->len);
  queue->len += 1;
  return p;
}

QUEUE_LINKAGE
QUEUE_TYPE Cat(QUEUE_FUNCTION_PREFIX, _pop)(QUEUE_NAME *queue) {
  Assert(queue->len > 0);
  QUEUE_TYPE res = queue->data[queue->head];
  queue->head = (queue->head + 1) & (queue->cap - 1);
  queue->len -= 1;
  return res;
}

QUEUE_LINKAGE
void Cat(QUEUE_FUNCTION_PREFIX, _append)(QUEUE_NAME *queue, Allocator allocator, QUEUE_TYPE item) {
  *Cat(QUEUE_FUNCTION_PREFIX, _push)(queue, allocator) = item;
}

QUEUE_LINKAGE
QUEUE_TYPE *Cat(QUEUE_FUNCTION_PREFIX, _ptr_at_unchecked)(QUEUE_NAME *queue, usize i) {
  return &queue->data[(queue->head + i) & (queue->cap - 1)];
}

QUEUE_LINKAGE
QUEUE_TYPE Cat(QUEUE_FUNCTION_PREFIX, _at_unchecked)(QUEUE_NAME *queue, usize i) {
  return *Cat(QUEUE_FUNCTION_PREFIX, _ptr_at_unchecked)(queue, i);
}

#undef QUEUE_OUTPUT_DEFINITIONS
#pragma clang diagnostic pop
#endif // QUEUE_OUTPUT_DEFINITIONS

#undef QUEUE_NAME
#undef QUEUE_TYPE
#undef QUEUE_FUNCTION_PREFIX
#undef QUEUE_MIN_SIZE_LOG2
#undef QUEUE_LINKAGE
