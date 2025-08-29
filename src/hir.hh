#pragma once

#include "blu.hh"

enum InstructionKind : u8 {
  i_mul,
  i_div,
  i_mod,
  i_sub,
  i_add,

  i_logical_and,
  i_logical_or,

  i_cmp_equal,
  i_cmp_less_than,

  i_store,
};

struct Instruction {
  InstructionKind kind;
  union {

  };
};
