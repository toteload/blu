#include "blu.hh"

#include <stdio.h>

void *stdlib_alloc_fn(void *ctx, void *p, usize old_byte_size, usize new_byte_size, u32 align) {
  if (!Is_null(p) && new_byte_size == 0) {
    free(p);
    return nullptr;
  }

  return realloc(p, new_byte_size);
}

static Allocator stdlib_alloc{stdlib_alloc_fn, nullptr};

Str read_file_aux(Arena *arena, Str filename) {
  char *buf = arena->alloc(filename.len + 1);
  memcpy(buf, filename.str, filename.len);
  buf[filename.len] = '\0';

  FILE *f = fopen(buf, "rb");
  if (!f) {
    return Str::empty();
  }

  fseek(f, 0, SEEK_END);

  u32 size   = ftell(f);
  char *data = Cast(char *, malloc(size + 1));
  if (!data) {
    return Str::empty();
  }

  fseek(f, 0, SEEK_SET);

  u64 bytes_read = fread(data, 1, size, f);
  if (bytes_read != size) {
    return Str::empty();
  }

  // Set a zero-terminated byte, but don't include it in the length.
  data[size] = '\0';

  return {data, size};
}

Str read_file(Arena *arena, Str filename) {
  auto snapshot = arena->take_snapshot();
  auto res = read_file_aux(arena, filename);
  arena->restore(snapshot);
  return res;
}

void CompilerContext::init() {
  arena.init(MiB(32));
  work_arena.init(MiB(32));
  messages.init(stdlib_alloc);
  strings.init(stdlib_alloc, stdlib_alloc, stdlib_alloc);
  types.init(&compiler_context.arena, stdlib_alloc, stdlib_alloc);
  nodes.init(stdlib_alloc);
  environments.init(stdlib_alloc, stdlib_alloc);
  sources.init(stdlib_alloc);
}

b32 CompilerContext::add_source_file_compile_job(Str filename) {
  Str source_text = read_file(&work_arena, source_filename);
  if (!source.is_ok()) {
    return false;
  }

  SourceFile *source = sources.push_empty();
  source->filename = filename;
  source->source = source_text;
  source->tokens.init(stdlib_alloc);

  b32 ok;
  ok = tokenize(&compiler_context, &source->tokens, source_text);
  if (!ok) {
    return false;
  }

  ok = parse(&compiler_context, source);
  if (!ok) {
    return false;
  }

  ok = run_builtins(&compiler_context, source);
  if (!ok) {
    return false;
  }

  ok = infer_types(&compiler_context, compiler_context.root);
  if (!ok) {
    return false;
  }
}
