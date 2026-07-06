#ifndef MESSAGES_H
#define MESSAGES_H

#include "blu.h"
#include <stdarg.h>

enum MessageSeverity {
  Severity_Error,
  Severity_Warning,
  Severity_Info,
};

enum MessageLocationKind {
  MessageLocation_unspecified,
  MessageLocation_end_of_file,
  MessageLocation_byte_offset,
  MessageLocation_token_index,
  MessageLocation_ast_index,
};

typedef struct {
  u8 kind;
  union {
    TokenIndex token_index;
    AstIndex   ast_index;
    u32        offset;
  } data;
} MessageLocation;

typedef union {
  u8         token_kind;
  AstIndex   ast_index;
  TypeIndex  type_index;
} MessageArg;

typedef struct {
  u8              severity;
  SourceIndex     source;
  MessageLocation location;
  String          format;
  MessageArg      args[];
} Message;

typedef Message* MessagePtr;

#define SEGMENTLIST_NAME          MessageList
#define SEGMENTLIST_TYPE          MessagePtr
#define SEGMENTLIST_MIN_SIZE_LOG2 6
#define SEGMENTLIST_SEGMENT_COUNT 24
#define SEGMENTLIST_OUTPUT_TYPES
#include "segment_list.h"

typedef struct {
  MessageList messages;
} Messages;

void     messages_init(Messages *messages);
u32      messages_count(Messages *messages);
Message *messages_get(Messages *messages, u32 idx);

void print_message(Message *message, Source *source);
void messages_print_all_messages(Messages *messages, SourceAllocator *sources);

void messages_errorv(Messages *messages, Arena *arena, SourceIndex source, MessageLocation location, String fmt, va_list args);

#endif // MESSAGES_H
