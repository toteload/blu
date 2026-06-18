#ifndef MESSAGES_H
#define MESSAGES_H

#include "blu.h"

enum MessageSeverity {
  Severity_Error,
  Severity_Warning,
  Severity_Info,
};

typedef struct {
  u8     severity;
  String format;
} Message;

#define SEGMENTLIST_NAME MessageList
#define SEGMENTLIST_TYPE Message
#define SEGMENTLIST_MIN_SIZE_LOG2 6
#define SEGMENTLIST_SEGMENT_COUNT 24
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

struct Messages {
  Arena       arena;
  MessageList messages;
};

void messages_init(Messages *messages);
void messages_print_all_messages(Messages *messages);

void messages_add_error(Messages *messages, String fmt);

#endif // MESSAGES_H
