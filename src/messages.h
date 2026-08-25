#ifndef MESSAGES_H
#define MESSAGES_H

#include "blu.h"

#include <stdarg.h>

typedef enum {
  Severity_Error,
  Severity_Warning,
  Severity_Info,
} MessageSeverity;

typedef enum {
  MessageLocation_unspecified,
  MessageLocation_byte_offset,
  MessageLocation_token_index,
  MessageLocation_ast_index,
  MessageLocation_ir_instruction,
} MessageLocationKind;

typedef struct {
  u8 kind;

  // `source_idx` is set if kind is byte_offset, token_index or ast_index.
  SourceIndex source_idx;

  // `decl_idx` is set if kind is ir_instruction.
  DeclarationIndex decl_idx;

  union {
    TokenIndex token_index;
    AstIndex ast_index;
    u32 offset;
  } data;
} MessageLocation;

typedef union {
  u8     token_kind;
  String string;
} MessageArg;

typedef struct {
  u8              severity;
  MessageLocation location;
  String          format;
  MessageArg      args[];
} Message;

typedef void (*FnAddMessage)(void *user, u8 severity, MessageLocation location, String format, ...);

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

// Format args support normal printf conversions plus two custom ones:
// - %tokenkind (u8 representing TokenKind)
// - %string    (String)
#define Message_error(psink, ...) (psink)->add_message((psink)->user, Severity_Error, __VA_ARGS__)

u32 message_format_arg_count(String fmt);
void message_collect_args(String format, va_list vl, MessageArg *args, u32 arg_count);
String message_format(Arena *scratch, Message *message);
void print_message(Arena *scratch, Message *message, Source *source, Declaration *decl);

#endif // MESSAGES_H
