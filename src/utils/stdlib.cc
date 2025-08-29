#include "stdlib.hh"
#include <stdio.h>

static void *stdlib_alloc_fn(void *ctx, void *p, usize old_byte_size, usize new_byte_size, u32 align) {
  if (!Is_null(p) && new_byte_size == 0) {
    free(p);
    return nullptr;
  }

  return realloc(p, new_byte_size);
}

Allocator stdlib_alloc{stdlib_alloc_fn, nullptr};

static Str read_file(Str filename) {
  char *buf = cast<char*>(malloc(filename.len() + 1));
  memcpy(buf, filename.str, filename.len());
  buf[filename.len()] = '\0';

  FILE *f = fopen(buf, "rb");
  if (!f) {
    free(buf);
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
    free(buf);
    return Str::empty();
  }

  // Set a zero-terminated byte, but don't include it in the length.
  data[size] = '\0';

  free(buf);
  return {data, size};
}

void write_file(Str filename, Str text) {
  char *buf = cast<char*>(malloc(filename.len() + 1));
  memcpy(buf, filename.str, filename.len());
  buf[filename.len()] = '\0';

  FILE *f = fopen(buf, "wb");
  fwrite(text.str, 1, text.len(), f);
  fclose(f);
  free(buf);
}

