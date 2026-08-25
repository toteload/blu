#include "messages.h"
#include "source_file.h"
#include "compiler.h"

#include <stdio.h>
#include <string.h>

#define SEGMENTLIST_NAME MessageList
#define SEGMENTLIST_TYPE MessagePtr
#define SEGMENTLIST_FUNCTION_PREFIX msglist
#define SEGMENTLIST_MIN_SIZE_LOG2 6
#define SEGMENTLIST_SEGMENT_COUNT 24
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

enum FormatSpecKind {
  FormatSpec_percent, // '%%', a literal '%'
  FormatSpec_token_kind,
  FormatSpec_string,
};

typedef struct {
  u8     kind;
  usize  start; // index of the '%' in the format string
  usize  end;   // index just past the directive
} FormatSpec;

internal b32 starts_with_at(String s, usize pos, String prefix) {
  if (pos + prefix.len > s.len) {
    return False;
  }
  return memcmp(s.str + pos, prefix.str, prefix.len) == 0;
}

// ASSUME: the format string is well formed.
internal b32 format_next_spec(String fmt, usize pos, FormatSpec *out) {
  for (usize i = pos; i < fmt.len; i++) {
    if (fmt.str[i] != '%') {
      continue;
    }

    out->start = i;

    if (i + 1 < fmt.len && fmt.str[i+1] == '%') {
      out->kind = FormatSpec_percent;
      out->end  = i + 2;
      return True;
    }

    usize j = i + 1;

    if (starts_with_at(fmt, j, string_lit("tokenkind"))) {
      out->kind = FormatSpec_token_kind;
      out->end  = j + 9;
      return True;
    }

    if (starts_with_at(fmt, j, string_lit("string"))) {
      out->kind = FormatSpec_string;
      out->end  = j + 6;
      return True;
    }

    Panic();
  }

  return False;
}

u32 message_format_arg_count(String fmt) {
  u32 count = 0;
  usize pos = 0;
  FormatSpec spec;
  while (format_next_spec(fmt, pos, &spec)) {
    if (spec.kind != FormatSpec_percent) {
      count++;
    }
    pos = spec.end;
  }
  return count;
}

void message_collect_args(String format, va_list vl, MessageArg *args, u32 arg_count) {
  usize pos = 0;
  u32 i = 0;
  FormatSpec spec;

  while (i < arg_count && format_next_spec(format, pos, &spec)) {
    pos = spec.end;

    switch (Cast(enum FormatSpecKind, spec.kind)) {
    case FormatSpec_percent: continue;
    case FormatSpec_token_kind: {
      args[i].token_kind = Cast(u8, va_arg(vl, int));
    } break;
    case FormatSpec_string: {
      args[i].string = va_arg(vl, String);
    } break;
    }

    i++;
  }
}

String message_format(Arena *scratch, Message *message) {
  String format = message->format;

  u32 bufsize = 100;
  u8 *buf = arena_push_array(u8, scratch, bufsize);

  usize len = 0;
  usize format_i = 0;
  u32 arg_i = 0;

  FormatSpec spec;
  while (format_next_spec(format, format_i, &spec)) {
    usize lit_len = spec.start - format_i;
    if ((len + lit_len) > bufsize) {
      arena_push_array(u8, scratch, lit_len);
    }

    memcpy(buf + len, format.str + format_i, lit_len);
    len += lit_len;
    format_i = spec.end;

    switch (Cast(enum FormatSpecKind, spec.kind)) {
    case FormatSpec_percent: {
      if ((len + 1) > bufsize) {
        arena_push_array(u8, scratch, 1);
      }

      buf[len++] = '%';
    } break;
    case FormatSpec_token_kind: {
      char const *s = token_kind_string(message->args[arg_i++].token_kind);
      u32 slen = strlen(s);
      if ((len + slen) > bufsize) {
        arena_push_array(u8, scratch, slen);
      }

      memcpy(buf + len, s, slen);
      len += slen;
    } break;
    case FormatSpec_string: {
      String s = message->args[arg_i++].string;
      if ((len + s.len) > bufsize) {
        arena_push_array(u8, scratch, s.len);
      }

      memcpy(buf + len, s.str, s.len);
      len += s.len;
    } break;
    }
  }

  u32 rest_len = format.len - format_i;
  if ((len + rest_len) > bufsize) {
    arena_push_array(u8, scratch, rest_len);
  }

  memcpy(buf + len, format.str + format_i, rest_len);
  len += rest_len;

  return (String){ .str = buf, .len = len };
}

internal b32 location_kind_has_line_col(u8 kind) {
  switch (Cast(MessageLocationKind, kind)) {
  case MessageLocation_ast_index:
  case MessageLocation_token_index:
  case MessageLocation_byte_offset:
  case MessageLocation_ir_instruction:
    return True;
  case MessageLocation_unspecified:
    return False;
  }
}

typedef struct {
  u32 line;
  u32 col;
  String textline;
  u32 underline_len;
} PositionInfo;

internal PositionInfo get_position_info(Source *source, Declaration *decl, MessageLocation loc) {
  u32 offset;
  u32 len = 0;
  switch (loc.kind) {
  case MessageLocation_byte_offset: {
    offset = loc.data.offset;
    len = 1;
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
  case MessageLocation_ir_instruction: {
    SIrChunk *chunk = &decl->data.decl.chunk;
    AstIndex ast_idx = chunk->sources[loc.data.offset].ast_idx;
    if (ast_idx) {
      SpanToken span_token = source->ast.spans[ast_idx];
      SpanU32 span_offset = source->tokens.spans[span_token.start];
      offset = span_offset.start;
      len = span_offset.end - span_offset.start;
    } else {
      return (PositionInfo){
        .line = 0,
        .col = 0,
        .textline = {0},
        .underline_len = 0,
      };
    }
  } break;
  case MessageLocation_unspecified: {
    if (decl) {
      Source *s = decl->data.decl.source;
      AstIndex ast_idx = s->decls[decl->data.decl.tree_idx].node;
      SpanToken span_token = source->ast.spans[ast_idx];
      SpanU32 span_offset = source->tokens.spans[span_token.start];
      offset = span_offset.start;
      len = span_offset.end - span_offset.start;
    } else {
      return (PositionInfo){
        .line = 0,
        .col = 0,
        .textline = {0},
        .underline_len = 0,
      };
    }
  } break;
  }

  LineInfo info = tokens_find_line_info(source->text, &source->tokens, offset);

  u32 line = info.line;
  u32 col = offset - info.offset_start_of_line + 1;

  // subtract 1 from the line length to exclude the newline
  String textline =
    (String){.str = source->text.str + info.offset_start_of_line, .len = info.line_len - 1};

  return (PositionInfo){
    .line = line,
    .col = col,
    .textline = textline,
    .underline_len = len,
  };
}

void print_message(Arena *scratch, Message *message, Source *source, Declaration *decl) {
  // clang-format off
  switch (Cast(MessageSeverity, message->severity)) {
  case Severity_Error:   printf("\033[1m[\033[31merror\033[39m]\033[22m"); break;
  case Severity_Warning: printf("[warn]");  break;
  case Severity_Info:    printf("[info]");  break;
  }
  // clang-format on

  if (decl) {
    source = decl->data.decl.source;
  }

  if (source) {
    String filename = source->filename;
    printf(" \033[1m%.*s:\033[22m", Cast(int, filename.len), filename.str);
  }

  PositionInfo info = {
    .line = 0,
    .col = 0,
    .textline = {0},
    .underline_len = 0,
  };

  if (location_kind_has_line_col(message->location.kind)) {
    info = get_position_info(source, decl, message->location);
    printf("\033[1m%u:%u:\033[22m", info.line, info.col);
  }

  ArenaSnapshot scope = arena_scope_begin(scratch);
  String formatted = message_format(scratch, message);
  printf(" %.*s\n", Cast(int, formatted.len), formatted.str);
  arena_scope_end(scratch, scope);

  if (info.line) {
    printf("%5u | %.*s\n", info.line, Cast(int, info.textline.len), info.textline.str);
    // Forgive me, Father, for I have sinned.
    printf(
      "      |%*c\033[32m%.*s\033[39m\n",
      info.col,
      ' ',
      Min(info.underline_len, 64),
      "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"
    );
  }
}
