#ifndef BYTECODE_H
#define BYTECODE_H

#include "blu.h"

enum BytecodeOp {
  CodeOp_int_add,
  CodeOp_branch,
  CodeOp_block,
  CodeOp_call,
  CodeOp_return,
};

typedef struct {

} BytecodeMachine;

#endif // BYTECODE_H 
