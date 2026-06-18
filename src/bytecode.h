#ifndef BYTECODE_H
#define BYTECODE_H

#include "blu.h"

enum InternalOp {
  InternalOp_typeid,
  InternalOp_emit,
  InternalOp_eval,
};

enum BytecodeOp {
  Op_add,
  Op_cond_branch,
  Op_block,
  Op_call,
  Op_return,

  Op_internal,
};

typedef struct {
} Bytecode;

typedef struct {
  Arena  registers;
  Arena  callstack;
  u32    pc;
  void  *ret_val_address;

  u8  *code;
  u32  code_len;
} BytecodeMachine;

void machine_init(BytecodeMachine *machine);

#endif // BYTECODE_H 
