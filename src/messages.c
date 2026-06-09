#include "messages.h"

#define SEGMENTLIST_NAME MessageList
#define SEGMENTLIST_TYPE Message
#define SEGMENTLIST_FUNCTION_PREFIX list
#define SEGMENTLIST_MIN_SIZE_LOG2 6
#define SEGMENTLIST_SEGMENT_COUNT 24
#define SEGMENTLIST_LINKAGE internal
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

void messages_init(Messages *messages) {
  arena_init(&messages->arena, &(ArenaOptions){
    .reserve_size = MiB(1),
    .initial_commit_size = KiB(2),
  });
  zero_struct(MessageList, &messages->messages);
}

void messages_add_error(Messages *messages, String fmt) {
  Message msg = {
    .severity = Severity_Error,
    .format   = arena_copy_string(&messages->arena, fmt),
  };
  list_append(&messages->messages, &messages->arena, msg);
}
