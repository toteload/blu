#include "messages.h"
#include "source_file.h"

#include <stdio.h>

#define SEGMENTLIST_NAME            MessageList
#define SEGMENTLIST_TYPE            MessagePtr
#define SEGMENTLIST_FUNCTION_PREFIX msglist
#define SEGMENTLIST_MIN_SIZE_LOG2   6
#define SEGMENTLIST_SEGMENT_COUNT   24
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

u32 message_format_arg_count(String fmt) {
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

internal b32 location_kind_has_line_col(u8 kind) {
  return kind == MessageLocation_token_index || kind == MessageLocation_ast_index || kind == MessageLocation_byte_offset;
}

typedef struct {
  u32 line;
  u32 col;
  String textline;
  u32 underline_len;
} PositionInfo;

internal PositionInfo get_position_info(Source *source, MessageLocation loc) {
  u32 offset;
  u32 len = 0;
  switch (loc.kind) {
  case MessageLocation_byte_offset: {
    offset = loc.data.offset;
  } break;
  case MessageLocation_token_index: {
    SpanU32 span_offset = source->tokens.spans[loc.data.token_index];
    offset = span_offset.start;
    len = span_offset.end - span_offset.start;
  } break;
  case MessageLocation_ast_index: {
    SpanToken span_token = source->ast.spans[loc.data.ast_index];
    SpanU32 span_offset = source->tokens.spans[span_token.start];
    offset = span_offset.start;
    len = span_offset.end - span_offset.start;
  } break;
  }

  LineInfo info = tokens_find_line_info(&source->tokens, offset);

  u32 line = info.line;
  u32 col  = offset - info.offset_start_of_line + 1;

  // subtract 1 from the line length to exclude the newline
  String textline = (String){ .str = source->text.str + info.offset_start_of_line, .len = info.line_len - 1 };

  return (PositionInfo){
    .line = line,
    .col = col,
    .textline = textline,
    .underline_len = len,
  };
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

  PositionInfo info = {
    .line = 0,
    .col = 0,
    .textline = {0},
    .underline_len = 0,
  };
  if (location_kind_has_line_col(message->location.kind)) {
    info = get_position_info(source, message->location);
    printf("%u:%u:", info.line, info.col);
  }

  printf(" %.*s\n", Cast(int, message->format.len), message->format.str);

  if (info.line) {
    printf("%5u | %.*s\n", info.line, Cast(int, info.textline.len), info.textline.str);
    // Forgive me, Father, for I have sinned.
    printf("      |%*c%.*s\n", info.col, ' ', Min(info.underline_len, 64), "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
  }
}
