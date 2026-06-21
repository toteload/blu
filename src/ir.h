#ifndef IR_H
#define IR_H

#include "blu.h"

typedef u32 IrRef; // references either an instruction or a value

enum IrOpcode {
  IR_func, // data references `IrFunc`
  IR_arg, // data contains `TypeIndex`
  IR_add,
  IR_call,
  IR_cond_br,
  IR_block,
  IR_br, // data contains `IrRef`
  IR_loop,
  IR_repeat,
  IR_ret, // data contains `IrRef`
  IR_load,
  IR_store,
  IR_cast, // data references `IrCast`

  IR_comptime_func,

  IR_typeid,

  // Emit blocks do not return a value and can be seen as a marker to the compiler on what code
  // to emit.
  IR_emit, // data contains how many instructions are in this block.

  // Eval blocks return a value and can only be exited with a `br`.
  IR_eval, // data contains how many instructions are in this block.
};

typedef struct {
  TypeIndex return_type;
  u32 arg_count;
  u32 instruction_count;
} IrFunc;

typedef struct {
  TypeIndex type;
  IrRef ref;
} IrCast;

typedef struct {
  u32   opcode_count;
  u8   *opcodes;
  u32  *data;
  void *extra;
} IrCode;

#endif // IR_H
