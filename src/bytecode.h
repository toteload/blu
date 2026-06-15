#ifndef BYTECODE_H
#define BYTECODE_H

#include "blu.h"

enum BytecodeOp {
  

  Op_add,

  Op_cond_branch,
  Op_block,
  Op_call,
  Op_return,
};

typedef struct {
  
} BytecodeMachine;

#endif // BYTECODE_H 
