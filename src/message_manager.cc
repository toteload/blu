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

    if ((i+1) < format.len() && format[i+1] == '{') {
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
			*arg = { format->str, i + 1 };
			*format = { format->str + i + 1, format->len() - i - 1 };
			return true;
		}
	}

	return false;
}

static char const *token_string[] = {
  "colon",        "arrow",        "semicolon",      "equals",      "minus",       "plus",
  "star",         "slash",        "percent",        "plus_equals", "exclamation", "ampersand",
  "bar",          "caret",        "tilde",          "left-shift",  "right-shift", "cmp-eq",
  "cmp-ne",       "cmp-gt",       "cmp-ge",         "cmp-lt",      "cmp-le",      "comma",
  "dot",          "literal_int",  "literal_string", "brace_open",  "brace_close", "paren_open",
  "paren_close",  "bracket_open", "bracket_close",  "fn",          "return",      "if",
  "else",         "while",        "break",          "continue",    "and",         "or",
  "for",          "in",           "cast",           "module",      "identifier",  "builtin",
  "line_comment", "<illegal>",
};

static char const *token_kind_string(u32 kind) {
  if (kind >= Tok_kind_max) {
    return token_string[Tok_kind_max];
  }

  return token_string[kind];
}

void write_message(Message *msg) {
  if (msg->severity == Error) {
    printf("[error] ");
  }

  Str format = msg->format;
  u32 arg_idx = 0;
  while (true) {
    usize offset = next_arg_offset(format);

    if (offset > 0) {
      printf("%.*s", cast<int>(offset), format.str);
    }

    format.str += offset;
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
    } else {
            printf("<UNRECOGNIZED %.*s>", cast<int>(arg.len()), arg.str);
    }

    arg_idx += 1;
  }

  printf("\n");
}

void MessageManager::write_messages() {
  for (usize i = 0; i < messages.len(); i++) {
    Message *msg = messages[i];
    write_message(msg);
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
