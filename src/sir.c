#include "sir.h"

// Specializer IR Builder
// -------------------------------------------------------------------------------------------------

InstructionOffset sir_builder_add(SIrBuilder *builder, u8 op, SourceKey source_idx, AstIndex ast_idx) {
  InstructionOffset idx = builder->kinds.len;
  opcodelist_append(&builder->kinds, builder->scratch, op);
  sourcelist_append(&builder->ast_source, builder->scratch, (AstAndSourceIndex){ .source_idx = source_idx, .ast_idx = ast_idx });
  datalist_append(&builder->data, builder->scratch, (InstData){ .ptr = Null });
  return idx;
}

SRef sir_builder_add_as(SIrBuilder *builder, SRef type_destination, SRef val, SourceKey source_idx, AstIndex ast_idx) {
  if (sref_is_nil(type_destination)) {
    return val;
  }

  InstructionOffset idx = sir_builder_add(builder, SIR_as, source_idx, ast_idx);

  SIrAs *data = sir_builder_push_data(builder, idx, SIrAs);
  *data = (SIrAs){
    .type_dst = type_destination,
    .val = val,
  };

  return sref_from_instruction(idx);
}

void *sir_builder_push_data_raw(SIrBuilder *builder, InstructionOffset idx, u32 size, u32 align) {
  void *p = arena_push(builder->scratch, size, align);
  *datalist_ptr_at_unchecked(&builder->data, idx) = (InstData){ .ptr = p };
  return p;
}

void sir_builder_set_data(SIrBuilder *builder, InstructionOffset idx, u32 data) {
  *datalist_ptr_at_unchecked(&builder->data, idx) = (InstData){ .data = data };
}

u8 sir_builder_get_opcode(SIrBuilder *builder, InstructionOffset idx) {
  return opcodelist_at_unchecked(&builder->kinds, idx);
}

InstructionOffset sir_builder_end_block_with(SIrBuilder *builder, InstructionOffset block, InstructionOffset target, SRef ref, SourceKey source_idx, AstIndex ast_idx) {
  InstructionOffset br = sir_builder_add(builder, SIR_br, source_idx, ast_idx);

  SIrBr *data = sir_builder_push_data(builder, br, SIrBr);
  *data = (SIrBr){
    .block = target,
    .value = ref,
  };

  sir_builder_end_block(builder, block);

  return br;
}

InstructionOffset sir_builder_add_block(SIrBuilder *builder, SRef result_type, SourceKey source_idx, AstIndex ast_idx) {
  InstructionOffset block = sir_builder_add(builder, SIR_block, source_idx, ast_idx);

  SIrBlock *data = sir_builder_push_data(builder, block, SIrBlock);
  *data = (SIrBlock){ .result_type = result_type };

  return block;
}

void sir_builder_end_block(SIrBuilder *builder, InstructionOffset block) {
  u32 count = sir_builder_offset(builder, block);

  if (sir_builder_get_opcode(builder, block) == SIR_block) {
    SIrBlock *data = datalist_at_unchecked(&builder->data, block).ptr;
    data->instruction_count = count;
  } else {
    sir_builder_set_data(builder, block, count);
  }
}

u32 sir_builder_offset(SIrBuilder *builder, InstructionOffset start) {
  u32 at = builder->kinds.len;
  return at - start;
}

internal b32 sir_opcode_references_extra(u8 op) {
  switch (Cast(SIrOpcode, op)) {
#define X(k,_1,e,_2) case k: return e;
#include "x_sir.h"
#undef X
  }
}

internal u32 sir_flex_array_size(u8 op, void *payload) {
  switch (Cast(SIrOpcode, op)) {
  case SIR_type: return Cast(SIrType*, payload)->arg_count * sizeof(SRef);
  case SIR_call: return Cast(SIrCall*, payload)->arg_count * sizeof(SRef);
  default:      return 0;
  }
}

internal u32 sir_extra_payload_size(u8 op, void *payload) {
  switch (Cast(SIrOpcode, op)) {
#define X(k,_1,_2,d) case k: return sizeof(d) + sir_flex_array_size(op, payload);
#include "x_sir.h"
#undef X
  }
}

internal u32 sir_extra_payload_align(u8 op) {
  switch (Cast(SIrOpcode, op)) {
#define X(k,_1,_2,d) case k: return Align_of(d);
#include "x_sir.h"
#undef X
  }
}

void sir_builder_flatten(SIrBuilder *builder, Arena *arena, SIrChunk *chunk) {
  u32 count = Cast(u32, builder->kinds.len);

  u8  *opcodes = arena_push_array(u8,  arena, count);
  AstAndSourceIndex *sources = arena_push_array(AstAndSourceIndex, arena, count);
  u32 *data    = arena_push_array(u32, arena, count);

  opcodelist_copy_to_array(&builder->kinds, opcodes);
  sourcelist_copy_to_array(&builder->ast_source, sources);

  void *extra = arena->at;

  for (InstructionOffset i = 0; i < count; i++) {
    u8 op = opcodes[i];
    InstData entry = datalist_at_unchecked(&builder->data, i);

    if (!sir_opcode_references_extra(op)) {
      data[i] = entry.data;
      continue;
    }

    u32 size  = sir_extra_payload_size(op, entry.ptr);
    u32 align = sir_extra_payload_align(op);
    void *mem = ir_flatten_push_data(extra, arena, data, i, size, align);
    memcpy(mem, entry.ptr, size);
  }

  *chunk = (SIrChunk){
    .opcode_count = count,
    .opcodes      = opcodes,
    .sources   = sources,
    .data         = data,
    .extra        = extra,
  };
}
