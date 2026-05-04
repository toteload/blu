#include "blu.hh"
#include <stdarg.h>
#include <stdio.h>

usize count_args(Str format) {
  // This function is not robust.
  // For now, I'm OK with that since it will only be used for strings we control.

  usize count = 0;

  for (usize i = 0; i < format.len(); i++) {
    if (format[i] == '}') {
      count++;
    }
  }

  return count;
}

usize next_arg_offset(Str format) {
  for (usize i = 0; i < format.len(); i++) {
    if (format[i] != '{') {
      continue;
    }

    if ((i + 1) < format.len() && format[i + 1] == '{') {
      i += 1;
      continue;
    }

    return i;
  }

  return format.len();
}

b32 parse_arg_from(Str *format, Str *arg) {
  for (usize i = 0; i < format->len(); i++) {
    if ((*format)[i] == '}') {
      *arg    = {format->str, i + 1};
      *format = {format->str + i + 1, format->len() - i - 1};
      return true;
    }
  }

  return false;
}

SourceLocation find_source_location(Str source, u32 offset) {
  u32 line = 1;
  u32 col  = 1;

  Assert(offset < source.len());

  for (u32 i = 0; i < offset; i++) {
    if (source[i] == '\n') {
      line++;
      col = 1;
    } else {
      col++;
    }
  }

  return {line, col};
}

void Messages::print_message(Message *msg) {
  switch (msg->severity) {
  case Error:
    printf("[error] ");
    break;
  default:
    Todo();
  }

  switch (msg->location.kind) {
  case MessageLocation_none:
    break;
  case MessageLocation_node_index: {
    auto token_span = source->nodes->span(msg->location.data.node_index);
    auto span       = source->tokens->span(token_span.start);
    auto loc        = find_source_location(source->source, span.start);
    printf("%d:%d: ", loc.line, loc.col);
  } break;
  case MessageLocation_token_index: {
    auto span = source->tokens->span(msg->location.data.token_index);
    auto loc  = find_source_location(source->source, span.start);
    printf("%d:%d: ", loc.line, loc.col);
  } break;
  }

  Str format  = Str::from_ptr_and_len(msg->format, msg->format_len);
  u32 arg_idx = 0;
  while (true) {
    usize offset = next_arg_offset(format);

    if (offset > 0) {
      printf("%.*s", cast<int>(offset), format.str);
    }

    format.str  += offset;
    format._len -= offset;

    if (format.is_empty()) {
      break;
    }

    Str arg;
    b32 ok = parse_arg_from(&format, &arg);

    if (!ok) {
      Todo();
    }

    if (str_eq(arg, Str_make("{tokenkind}"))) {
      printf("<%s>", token_kind_string(msg->args[arg_idx].token_kind));
    } else if (str_eq(arg, Str_make("{type}"))) {
      char buf[256]  = {0};
      TypeIndex type = msg->args[arg_idx].type;
      u32 len        = type_to_string(types, type, buf, 256);
      printf("%.*s", cast<int>(len), buf);
    } else if (str_eq(arg, Str_make("{strkey}"))) {
      StrKey key = msg->args[arg_idx].strkey;
      Str str    = strings->get(key);
      printf("%.*s", cast<int>(str.len()), str.str);
    } else if (str_eq(arg, Str_make("{span}"))) {
      Span<u32> span = msg->args[arg_idx].span;
      auto loc       = find_source_location(source->source, span.start);
      printf("%d:%d", loc.line, loc.col);
    } else {
      printf("<UNRECOGNIZED %.*s>", cast<int>(arg.len()), arg.str);
    }

    arg_idx += 1;
  }

  printf("\n");
}

void Messages::print_messages() {
  for (usize i = 0; i < messages.len(); i++) {
    print_message(messages[i]);
  }
}

void Messages::_error(MessageLocation location, char const *format, va_list varargs) {
  u32 format_len = strlen(format);
  usize count    = count_args(Str::from_ptr_and_len(format, format_len));

  usize format_byte_size = format_len + 1;
  // We add 1 to the count to ensure we have enough for alignment of the args.
  usize byte_size = sizeof(Message) + format_byte_size + (count + 1) * sizeof(MessageArg);

  Message *msg = cast<Message *>(arena.raw_alloc(byte_size, Align_of(Message)));

  char *fmt_buf    = cast<char *>(msg + 1);
  MessageArg *args = cast<MessageArg *>(
    ptr_forward_align(ptr_offset(fmt_buf, format_byte_size), Align_of(MessageArg))
  );

  *msg = {
    .location   = location,
    .severity   = Error,
    .format_len = format_len,
    .format     = fmt_buf,
    .args       = args,
  };

  memcpy(fmt_buf, format, format_byte_size);

  for (usize i = 0; i < count; i++) {
    args[i] = va_arg(varargs, MessageArg);
  }

  messages.push(msg);
}

void Messages::error(char const *format, ...) {
  va_list varargs;
  va_start(varargs, format);
  _error(
    {
      .kind = MessageLocation_none,
      .data = {},
    },
    format,
    varargs
  );
  va_end(varargs);
}

void Messages::error(TokenIndex location, char const *format, ...) {
  va_list varargs;
  va_start(varargs, format);
  _error(
    {
      .kind = MessageLocation_token_index,
      .data =
        {
          .token_index = location,
        },
    },
    format,
    varargs
  );
  va_end(varargs);
}
void Messages::error(NodeIndex location, char const *format, ...) {
  va_list varargs;
  va_start(varargs, format);
  _error(
    {
      .kind = MessageLocation_node_index,
      .data = {.node_index = location},
    },
    format,
    varargs
  );
  va_end(varargs);
}
