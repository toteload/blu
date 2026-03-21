#include "blu.hh"

struct StringWriter {
  Arena *arena;

  void vprintf(char const *format, va_list arg) {
    int len   = vsnprintf(nullptr, 0, format, arg)+1;
    char *buf = arena->alloc<char>(len);
    vsnprintf(buf, len, format, arg);

    // Kinda ugly.
    // vsnprintf always writes a null terminator even though we don't want that.
    // We allocate enough memory for the string + the null terminator, then we move the pointer
    // one byte back so that next time the null terminator gets overwritten.
    arena->at = ptr_offset(arena->at, -1);
  }

  void write_to_file(char const *filename) {
    FILE *f = fopen(filename, "w");
    fwrite(arena->base, 1, arena->size(), f);
    fclose(f);
  }
};

struct CodeGenerator {
  StringWriter writer;

  void output_toplevel_declaration();
};

void CodeGenerator::output_toplevel_declaration(HirIndex idx) {
  auto inst = code->get(idx);

  auto type = code->get({idx.idx + 1});

}

b32 generate_c_code_from_hir(HirCode *code) {
  CodeGenerator gen;

  for (u32 i = 0; i < code->len(); i++) {
    
  }

  return true;
}
