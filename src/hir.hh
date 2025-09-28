#pragma once

#include "blu.hh"

enum InstructionKind : u8 {
  hir_root,

  hir_declaration,

  hir_function,
  hir_parameter,

  hir_sub,
  hir_add,
  hir_mul,
  hir_div,
  hir_mod,

  hir_logical_and,
  hir_logical_or,

  hir_cmp_equal,
  hir_cmp_less_than,

  hir_alloc,

  hir_load,
  hir_store,

  hir_call,

  hir_loop,
  hir_return,
};

// Can refer to either
// - another instruction
// - an entry into the value store
// - a declaration
struct Ref {
  u32 idx;
};

struct Root {
  SegmentList<Ref> declarations;
};

struct Declaration {
  NodeIndex ast;
  Ref type;
  Ref value;
};

struct Function {
  Ref return_type;
  u32 parameter_count;
  u32 body_instruction_count;
};

struct Parameter {
  Ref type;
};

struct CondBr {};

struct HirCode {
  Vector<u32> declarations;
  Vector<HirInstructionKind> kinds;
  Vector<HirData> datas;
  Vector<u32> extras;
};

b32 generate_hir(Source *src, HirCode *code);
