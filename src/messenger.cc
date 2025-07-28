#include "blu.cc"
#include <stdarg.h>

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

// It is assumed that format will be a constant string and thus does not need to be copied.
void MessageManager::log(
  MessageSeverity severity,
  SourceIdx src_idx,
  SourceSpan span,
  char const *format,
  ...)
{
  va_list args;
  va_start(args, format);

  Message *msg = arena.alloc<Message>();
  msg->severity = severity;
  msg->src_idx = src_idx;
  msg->span = span;

  Str fmt = Str::from_cstr(format);
  msg->format = fmt;

  usize count = count_args(fmt);

  MessageArg *args = arena.alloc<MessageArg>(count);

  for (usize i = 0; i < count; i++) {
    args[i] = va_arg(args, MessageArg);
  }

  va_end(args);

  messages.push(msg);
}
