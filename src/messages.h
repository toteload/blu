#ifndef MESSAGES_H
#define MESSAGES_H

#include "blu.h"

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

typedef void (*FnAddMessage)(void *user, u8 severity, SourceIndex source, MessageLocation location, String format, ...);

typedef struct {
  void *user;
  FnAddMessage add_message;
} MessageSink;

typedef Message* MessagePtr;

#define SEGMENTLIST_NAME          MessageList
#define SEGMENTLIST_TYPE          MessagePtr
#define SEGMENTLIST_FUNCTION_PREFIX msglist
#define SEGMENTLIST_MIN_SIZE_LOG2 6
#define SEGMENTLIST_SEGMENT_COUNT 24
#define SEGMENTLIST_OUTPUT_TYPES
#define SEGMENTLIST_OUTPUT_DECLARATIONS
#include "segment_list.h"

#define Message_error(psink, ...) (psink)->add_message((psink)->user, Severity_Error, __VA_ARGS__)

u32 message_format_arg_count(String fmt);
void print_message(Message *message, Source *source);

#endif // MESSAGES_H
