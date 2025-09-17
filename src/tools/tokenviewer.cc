#include <stdarg.h>
#include "blu.hh"
#include "utils/stdlib.hh"

char const *html_template = R"(<!DOCTYPE html>
<html>
  <head>
<style>
    * {
    margin: 0;
    }

    body {
    padding: 16px;
    }

    #main {
    display: flex;
    flex-direction: row;
    align-items: flex-start;
    gap: 8px;
    }

    code {
    white-space: pre;
    font-family: monospace;
    }

    #source-code {
    padding: 8px;
    border: 1px solid;
    }

    #token-list {
    display: grid;
    grid-column-gap: 8px;
    grid-template-columns: repeat(3, auto);
    align-content: start;

    font-family: monospace;
    padding: 8px;
    border: 1px solid;
    list-style-type: none;

    li {
    grid-column: 1 / span 3;
    display: grid;
    grid-template-columns: subgrid;
    }
    }
    </style>
  </head>
  <body>
    <div id="main">
    %.*s
    %.*s
    </div>
<script>
const colors = [];
for (var i = 0; i < 7; i++) {
  colors.push(`oklch(0.99 55%% ${(i * 151.0) %% 360.0})`);
}
const tokenCount = %d;
for (var i = 0; i < tokenCount; i++) {
  const query = `.a${String(i).padStart(%d, '0')}`
  const pair = document.querySelectorAll(query);
  const color = colors[i%%colors.length];
  for (const e of pair) {
    pair[0].style.background = color;
    e.addEventListener('mouseenter', () => {
      pair[0].style.outline = 'solid 1px';
      pair[1].style.outline = 'solid 1px';
      pair[1].style.background = color;
    });
    e.addEventListener('mouseleave', () => {
      pair[0].style.outline = 'none';
      pair[1].style.outline = 'none';
      pair[1].style.background = 'none';
    });
  }
}
    </script>
  </body>
</html>)";

struct StringBuilder {
  Allocator _alloc;
  Vector<char> buf;

  void init(Allocator alloc) {
    _alloc = alloc;
    buf.init(alloc);
  }

  void push(Str s) {
    buf.ensure_free_capacity(s.len());
    memcpy(buf.end(), s.str, s.len());
    buf._len += s.len();
  }

  void push_format(char const *format, ...) {
    va_list varargs;
    va_start(varargs, format);
    i32 size = vsnprintf(nullptr, 0, format, varargs);
    va_end(varargs);

    buf.ensure_free_capacity(size+1);

    va_start(varargs, format);
    vsnprintf(buf.end(), size+1, format, varargs);
    va_end(varargs);

    buf._len += size;
  }

  Str str() {
    char *s = _alloc.alloc<char>(buf.len());
    memcpy(s, buf.data, buf.len());
    return { s, buf.len() };
  }
};

Str build_token_html_view(Str source, Tokens *tokens) {
  Str html_source;
  {
    StringBuilder builder;
    builder.init(stdlib_alloc);
    builder.push(Str_make(R"(<code id="source-code">)"));
    usize at = 0;
    for (usize i = 0; i < tokens->kinds.len(); i++) {
      Span span = tokens->spans[i];

      if (at != span.start) {
        builder.push_format("%.*s", cast<u32>(span.start - at), source.str + at);
        at = span.start;
      }

      builder.push_format(
        R"(<span class="a%06d">%.*s</span>)",
        i,
        cast<u32>(span.end - span.start),
        source.str + at
      );

      at = span.end;
    }

    if (at != source.len()) {
      builder.push_format("%.*s", cast<u32>(at - source.len()), source.str + at);
    }

    builder.push(Str_make(R"(</code>)"));
    html_source = builder.str();
  }

  Str html_tokenlist;
  {
    StringBuilder builder;
    builder.init(stdlib_alloc);
    builder.push(Str_make(R"(<ul id="token-list">)"));
    for (usize i = 0; i < tokens->kinds.len(); i++) {
      Span span = tokens->spans[i];
      Str code = source.sub(span.start, span.end);
      builder.push_format(
        R"(<li class="a%06d"><span>%d:%d</span><span>%s</span><code>%.*s</code></li>)",
        i,
        span.start,
        span.end,
        token_kind_string(tokens->kinds[i]),
        cast<u32>(code.len()),
        code.str
      );
    }
    builder.push(Str_make(R"(</ul>)"));
    html_tokenlist = builder.str();
  }

  StringBuilder builder;
  builder.init(stdlib_alloc);
  builder.push_format(
    html_template,
    cast<u32>(html_source.len()),
    html_source.str,
    cast<u32>(html_tokenlist.len()),
    html_tokenlist.str,
    cast<u32>(tokens->kinds.len()),
    6
  );

  return builder.str();
}

i32 main(i32 arg_count, char const *const *args) {
  if (arg_count < 2) {
    printf("No file provided\n");
    return 1;
  }

  Arena arena;
  arena.init(MiB(8));

  Str filename = Str::from_cstr(args[1]);
  Str source = read_file(filename);
  if (source.is_empty()) {
    printf("Problem occured reading file\n");
    return 1;
  }

  StringInterner strings;
  strings.init(arena.as_allocator(), stdlib_alloc, stdlib_alloc);

  MessageManager messages;
  messages.init(stdlib_alloc, &strings);

  Tokens tokens;
  tokens.kinds.init(stdlib_alloc);
  tokens.spans.init(stdlib_alloc);
  b32 ok = tokenize(&messages, source, &tokens);
  if (!ok) {
    printf("Error while tokenizing\n");
    return 1;
  }

  Str html = build_token_html_view(source, &tokens);
  write_file(Str_make("tokens.html"), html);

  return 0;
}
