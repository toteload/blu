#include "messages.h"
#include "source_file.h"

#include <stdio.h>
#include <stdarg.h>

#define SEGMENTLIST_NAME            MessageList
#define SEGMENTLIST_TYPE            MessagePtr
#define SEGMENTLIST_FUNCTION_PREFIX list
#define SEGMENTLIST_MIN_SIZE_LOG2   6
#define SEGMENTLIST_SEGMENT_COUNT   24
#define SEGMENTLIST_LINKAGE         internal
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

void messages_init(Messages *messages) {
  zero_struct(MessageList, &messages->messages);
}

u32 messages_count(Messages *messages) {
  return messages->messages.len;
}

Message *messages_get(Messages *messages, u32 idx) {
  return *list_ptr_at_unchecked(&messages->messages, idx);
}

internal u32 format_arg_count(String fmt) {
  // ASSUME: the format string is well formed.
  u32 count = 0;
  for (usize i = 0; i < fmt.len; i++) {
    u8 c = fmt.str[i];
    if (c == '{') {
      if (i + 1 < fmt.len && fmt.str[i + 1] == '{') {
        // Escaped '{{' — skip the second brace.
        i++;
      } else {
        // Start of an argument; skip to the closing '}'.
        count++;
        while (i < fmt.len && fmt.str[i] != '}') {
          i++;
        }
      }
    } else if (c == '}') {
      // Escaped '}}' — skip the second brace.
      i++;
    }
  }
  return count;
}

void messages_errorv(Messages *messages, Arena *arena, SourceIndex source, MessageLocation location, String fmt, va_list args) {
  u32 arg_count = format_arg_count(fmt);

  Message *msg = arena_push(arena, sizeof(Message) + arg_count * sizeof(MessageArg), Align_of(Message));

  msg->severity = Severity_Error;
  msg->source   = source;
  msg->location = location;
  msg->format   = arena_copy_string(arena, fmt);

  for (u32 i = 0; i < arg_count; i++) {
    msg->args[i] = va_arg(args, MessageArg);
  }

  list_append(&messages->messages, arena, msg);
}

internal b32 location_kind_has_line_col(u8 kind) {
  return kind == MessageLocation_token_index || kind == MessageLocation_ast_index || kind == MessageLocation_byte_offset;
}

internal void get_line_col(Source *source, MessageLocation loc, u32 *line, u32 *col, String *textline) {
  u32 offset;
  switch (loc.kind) {
  case MessageLocation_byte_offset: {
    offset = loc.data.offset;
  } break;
  case MessageLocation_token_index: {
    SpanU32 span_offset = *tokens_span(&source->tokens, loc.data.token_index);
    offset = span_offset.start;
  } break;
  case MessageLocation_ast_index: {
    SpanToken span_token = *nodes_span(&source->ast, loc.data.ast_index);
    SpanU32 span_offset = *tokens_span(&source->tokens, span_token.start);
    offset = span_offset.start;
  } break;
  }

  LineInfo info = tokens_find_line_info(&source->tokens, offset);

  *line = info.line;
  *col  = offset - info.offset_start_of_line + 1;

  // subtract 1 from the line length to exclude the newline
  *textline = (String){ .str = source->text.str + info.offset_start_of_line, .len = info.line_len - 1 };
}

void print_message(Message *message, Source *source) {
  // clang-format off
  switch (message->severity) {
  case Severity_Error:   printf("[error]"); break;
  case Severity_Warning: printf("[warn]");  break;
  case Severity_Info:    printf("[info]");  break;
  }
  // clang-format on

  String filename = source->filename;
  printf(" %.*s:", Cast(int, filename.len), filename.str);

  u32 line = 0;
  u32 col = 0;
  String textline;
  if (location_kind_has_line_col(message->location.kind)) {
    get_line_col(source, message->location, &line, &col, &textline);
    printf("%u:%u:", line, col);
  }

  printf(" %.*s\n", Cast(int, message->format.len), message->format.str);

  if (line) {
    printf("%5u | %.*s\n", line, Cast(int, textline.len), textline.str);
    printf("      |%*c^\n", col, ' ');
  }
}

void messages_print_all_messages(Messages *messages, SourceAllocator *sources) {
  for (u32 i = 0; i < messages->messages.len; i++) {
    Message *msg = *list_ptr_at_unchecked(&messages->messages, i);
    Source *source = sources_get(sources, msg->source);
    print_message(msg, source);
  }
}
