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

void MessageManager::write_messages() {
  for (usize i = 0; i < messages.len(); i++) {
    Message *msg = messages[i];

    if (msg->severity == Error) {
      printf("[error] ");
    }

    printf("%s\n", msg->format.str);
  }
}

// It is assumed that format will be a constant string and thus does not need to be copied.
void MessageManager::error(
  char const *format,
  ...)
{
  va_list varargs;
  va_start(varargs, format);

  Message *msg = arena.alloc<Message>();
  msg->severity = Error;

  Str fmt = Str::from_cstr(format);
  msg->format = fmt;

  usize count = count_args(fmt);

  MessageArg *args = arena.alloc<MessageArg>(count);

  for (usize i = 0; i < count; i++) {
    args[i] = va_arg(varargs, MessageArg);
  }

  va_end(varargs);

  messages.push(msg);
}
