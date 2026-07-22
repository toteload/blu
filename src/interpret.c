#include "interpret.h"
#include "ir.h"

typedef struct {
  IrChunk *ir;
  IrBuilder builder;
} Interpreter;

b32 source_interpret_code(InterpretContext *context, Source *source, u32 idx_declaration) {
  Panic();
  return False;
}
