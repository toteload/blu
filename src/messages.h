#ifndef MESSAGES_H
#define MESSAGES_H

#include "blu.h"

enum MessageSeverity {
  Severity_Error,
  Severity_Warning,
  Severity_Info,
};

enum MessageLocationKind {
  MessageLocation_unknown,
  MessageLocation_token_index,
  MessageLocation_ast_index,
};

typedef struct {
  u8 kind;
  SourceFile *source;
  union {
    TokenIndex token_index;
    AstIndex   ast_index;
  } data;
} MessageLocation;

typedef union {
  TokenIndex token_index;
  AstIndex   ast_index;
  TypeIndex  type_index;
} MessageArg;

typedef struct {
  u8              severity;
  MessageLocation location;
  String          format;
  MessageArg      args[];
} Message;

#define SEGMENTLIST_NAME          MessageList
#define SEGMENTLIST_TYPE          Message
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

void messages_error(Messages *messages, MessageLocation location, String fmt, ...);

#endif // MESSAGES_H
