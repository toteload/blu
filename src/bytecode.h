#ifndef BYTECODE_H
#define BYTECODE_H

#include "blu.h"

enum VMOpcode {
  VM_add,
  VM_alloc,
  VM_func,
  VM_load,
  VM_store,
  VM_cast,
  VM_br,

  VM_emit,
  VM_eval,
};

// emit
//   eval
//     const %0 (uint 0)
//   @loop0
//     const %8 (uint 1)
//     add (uint) %16, %0, %8
//     cmp (uint) eq %17, %16, %arg_0
//     jmpif %17, @loop0_end
//     mov (uint) %0, %16
//     emit
//       call std.println "hello {}" (uint %0)
//     jmp @loop0
//   @loop0_end

// emit
//   eval
//     load_const %16 (const i128 0)
//     cast i32_from_i128 %0 %16
//   ret (uint %0)

typedef struct {
  u32  opcode_count;
  u8   *opcodes;
  u32  *data;
  void *extra;
} VmCode;

typedef struct {

} VirtualMachine;

void machine_init(VirtualMachine *machine);

#endif // BYTECODE_H 
