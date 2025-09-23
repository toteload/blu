#pragma once

#include "blu.hh"

enum InstructionKind : u8 {
  hir_root,

  hir_declaration,

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

struct CondBr {};
