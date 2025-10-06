#pragma once

#include "blu.hh"

enum HirInstructionKind : u8 {
  Hir_declaration,

  Hir_function,
  Hir_parameter,

  Hir_sub,
  Hir_add,
  Hir_mul,
  Hir_div,
  Hir_mod,

  Hir_logical_and,
  Hir_logical_or,

  Hir_cmp_equal,
  Hir_cmp_less_than,

  Hir_alloc,

  Hir_load,
  Hir_store,

  Hir_call,

  Hir_loop,
  Hir_return,
};

enum RefKind {
  Ref_instruction_index,
  Ref_value,
};

// Can refer to either
// - another instruction
// - an entry into the value store
struct Ref {
  u32 idx;
};

struct HirDeclaration {
  NodeIndex ast;
  Ref type;
  Ref value;
};

struct HirFunction {
  Ref return_type;
  u32 parameter_count;
  u32 body_instruction_count;
};

struct HirParameter {
  Ref type;
};

struct HirCondBr {};

union HirData {
  HirDeclaration declaration;
  HirFunction function;
  HirParameter parameter;
};

struct HirCode {
  Vector<HirInstructionKind> kinds;
  Vector<HirData>            datas;
  Vector<u32>                extras;

  Ref add(HirInstructionKind kind, HirData data) {
    u32 idx = cast<u32>(kinds.len());
    kinds.push(kind);
    datas.push(data);
    return {idx};
  }
};

b32 generate_hir(Source *src, HirCode *code);
