#include "messages.h"

#include <stdio.h>

#define SEGMENTLIST_NAME            MessageList
#define SEGMENTLIST_TYPE            Message
#define SEGMENTLIST_FUNCTION_PREFIX list
#define SEGMENTLIST_MIN_SIZE_LOG2   6
#define SEGMENTLIST_SEGMENT_COUNT   24
#define SEGMENTLIST_LINKAGE         internal
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

void messages_init(Messages *messages) {
  arena_init(&messages->arena, &(ArenaOptions){
    .reserve_size = MiB(1),
    .initial_commit_size = KiB(2),
  });
  zero_struct(MessageList, &messages->messages);
}

void messages_error(Messages *messages, String fmt, ...) {
  Message msg = {
    .severity = Severity_Error,
    .format   = arena_copy_string(&messages->arena, fmt),
  };
  list_append(&messages->messages, &messages->arena, msg);
}

internal void print_message(Message *message) {
  switch (message->severity) {
  case Severity_Error:   printf("[error]"); break;
  case Severity_Warning: printf("[warn]"); break;
  case Severity_Info:    printf("[info]"); break;
  }

  printf(" %.*s\n", Cast(int, message->format.len), message->format.str);
}

void messages_print_all_messages(Messages *messages) {
  for (u32 i = 0; i < messages->messages.len; i++) {
    print_message(list_ptr_at_unchecked(&messages->messages, i));
  }
}
