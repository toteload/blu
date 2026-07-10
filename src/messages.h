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

typedef Message* MessagePtr;

typedef void (*FnAddMessage)(void *user, u8 severity, MessageLocation location, String format, ...);

typedef struct {
  void *user;
  FnAddMessage add_message;
} MessageSink;

#define Message_error(psink, loc, fmt, ...) \
  (psink)->add_message((psink)->user, Severity_Error, loc, fmt, ##__VA_ARGS__)

u32 message_format_arg_count(String fmt);
void print_message(Message *message, Source *source);

#endif // MESSAGES_H
