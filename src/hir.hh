#pragma once

#include "blu.hh"
#include "utils/stdlib.hh"

enum HirInstructionKind : u8 {
  Hir_root,
  Hir_declaration,

  // Constant values
  Hir_literal_int,
  Hir_true,
  Hir_false,
  Hir_nil,

  Hir_declared_value,

  // Types
  Hir_type_int,
  Hir_type_nil,
  Hir_type_function,

  Hir_function,
  Hir_param,

  Hir_block,

  Hir_sub,
  Hir_add,
  Hir_mul,
  Hir_div,
  Hir_mod,

  Hir_cmp_eq,
  Hir_cmp_le,

  Hir_condbr,

  Hir_alloc,

  Hir_load,
  Hir_store,

  Hir_call,

  Hir_loop,
  Hir_break,
  Hir_repeat,
  Hir_return,

  Hir_kind_max,
};

struct HirExtraIndexTag {};
using HirExtraIndex = Index<u32, HirExtraIndexTag>;

struct HirTag {};
using HirIndex = Index<u32, HirTag>;

constexpr HirIndex INVALID_HIR_INDEX = { UINT32_MAX };

// The `function` instruction is always followed by n `param` instructions.
struct HirFunction {
  u32 param_count;
  u32 instruction_count;
};

struct HirTypeFunction {
  u32 param_count;
  HirIndex return_type;
};

struct HirParameter { };

struct HirCondBr {
  HirIndex cond;
  u32 then_instruction_count;
};

struct HirBlock {
  u32 instruction_count; // excluding self
};

struct HirBinary {
  HirIndex lhs;
  HirIndex rhs;
};

struct HirCall {
  HirIndex callee;
  HirExtraIndex extra_index;
};

struct HirBreak {
  HirIndex value;
  HirIndex block;
};

// The type of the declaration is immediately after this instruction.
// The value of the declaration is at `value`.
// The total size of this declaration (including itself) is `instruction_count`.
struct HirDeclaration {
  HirIndex value;
  u32 instruction_count;
};

struct HirTypeInt {
  Signedness signedness;
  u16 bitwidth;
};

union HirData {
  HirBlock       root;
  HirDeclaration declaration;
  HirFunction    function;
  HirParameter   parameter;
  HirCondBr      condbr;
  HirBlock       block;
  HirBinary      binary;
  HirBreak       break_;
  HirTypeInt     type_int;
  HirTypeFunction type_function;
  i64            int64;
  HirIndex       idx;
};

struct HirInstruction {
  HirInstructionKind *kind;
  HirData *data;
  OptionalTypeIndex *type;
};

struct HirCode {
  Vector<HirInstructionKind> kinds;
  Vector<HirData>            datas;
  Vector<OptionalTypeIndex>  types;
  Vector<TokenIndex>         token;
  Vector<u32>                extra;

  void init(Allocator alloc) {
    kinds.init(alloc);
    datas.init(alloc);
    types.init(alloc);
    token.init(alloc);
    extra.init(alloc);
  }

  void deinit() {
    kinds.deinit();
    datas.deinit();
    types.deinit();
    token.deinit();
    extra.deinit();
  }

  u32 len() {
    return cast<u32>(kinds.len());
  }

  OptionalTypeIndex type(HirIndex idx) {
    return types[idx.idx];
  }

  HirIndex alloc() {
    HirIndex res = {cast<u32>(kinds.len())};
    kinds.push_empty();
    datas.push_empty();
    return res;
  }

  HirIndex add(HirInstructionKind kind, HirData data) {
    u32 idx = cast<u32>(kinds.len());
    kinds.push(kind);
    datas.push(data);
    return {idx};
  }

  HirExtraIndex alloc_extra(u32 amount) {
    HirExtraIndex idx = {cast<u32>(extra.len())};
    extra.push_empty(amount);
    return idx;
  }

  u32 *get_extra_ptr(HirExtraIndex idx) {
    return &extra[idx.inner()];
  }

  ttld_inline HirInstruction get(HirIndex idx) {
    return {
      .kind = &kinds[idx.idx],
      .data = &datas[idx.idx],
      .type = &types[idx.idx],
    };
  }

  HirData *data_ptr(HirIndex idx) { return &datas[idx.idx]; }
};

constexpr char const *hir_string[Hir_kind_max] = {
  "root",
  "declaration",
  "literal_int",
  "true",
  "false",
  "nil",
  "declared_value",
  "type_int",
  "type_nil",
  "type_function",
  "function",
  "param",
  "block",
  "sub",
  "add",
  "mul",
  "div",
  "mod",
  "cmp_eq",
  "cmp_le",
  "condbr",
  "alloc",
  "load",
  "store",
  "call",
  "loop",
  "break",
  "repeat",
  "return",
};

ttld_inline char const *hir_kind_string(u32 kind) {
  if (kind >= Hir_kind_max) {
    return "illegal";
  }

  return hir_string[kind];
}

