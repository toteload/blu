#include "ir.h"

#define Bitmask_ir_ref_is_value_index (Cast(u32, 1) << 31)

u32 code_index_hash(void *context, IrCodeIndex idx) {
  Unused(context);

  return idx;
}

b32 code_index_eq(void *context, IrCodeIndex a, IrCodeIndex b) {
  Unused(context);

  return a == b;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#define HASHMAP_NAME            InstructionResultMap
#define HASHMAP_FUNCTION_PREFIX map
#define HASHMAP_KEY_TYPE        IrCodeIndex
#define HASHMAP_VALUE_TYPE      ValueIndex
#define HASHMAP_HASH_FN         code_index_hash
#define HASHMAP_KEY_COMPARE_FN  code_index_eq
#define HASHMAP_LINKAGE         internal
#define HASHMAP_OUTPUT_DEFINITIONS
#include "hashmap.h"
#pragma clang diagnostic pop

#define SEGMENTLIST_NAME            CallStackList
#define SEGMENTLIST_TYPE            IrLocation
#define SEGMENTLIST_MIN_SIZE_LOG2   5
#define SEGMENTLIST_SEGMENT_COUNT   24
#define SEGMENTLIST_FUNCTION_PREFIX callstack
#define SEGMENTLIST_LINKAGE         internal
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

internal always_inline IrInstructionIndex ref_to_instruction_index(IrRef ref) {
  return ref;
}

internal always_inline IrRef instruction_index_to_ref(IrInstructionIndex idx) {
  return idx;
}

internal always_inline b32 ref_is_value_index(IrRef ref) {
  return (ref & Bitmask_ir_ref_is_value_index) != 0;
}

u8 opcode(IrChunk *chunk, IrInstructionIndex idx) {
  return chunk->opcodes[idx];
}

u32 instruction_data(IrChunk *chunk, IrInstructionIndex idx) {
  return chunk->data[idx];
}

void *instruction_extra(IrChunk *chunk, IrInstructionIndex idx) {
  return ptr_offset(chunk->extra, chunk->data[idx]);
}

Value *ir_ref_value(IrMachine *machine, IrRef ref) {
  if (ref_is_value_index(ref)) {
    return values_get(machine->values, ref_to_value_index(ref));
  }

  return map_find(&machine->inst_map, ref_to_code_index(ref));
}

internal u32 ir_eval(IrMachine *machine) {
  IrLocation pc = *callstack_ptr_at_unchecked(&machine->callstack, machine->callstack.len-1);
  IrChunk *chunk = get_chunk(machine->chunks, pc.chunk_index);

  while (True) {
    op = opcode(chunk, pc.instruction_index);

    switch (op) {
    case IR_alloc: {
      TypeIndex type_idx = instruction_data(chunk, pc);
      Type *type = types_get(types, type_idx);
      TypeSizeInfo size_info = types_size_info_by_type(types, type);
      void *mem = values_alloc_data(values, size_info.size, size_info.align);
      Value *val;
      ValueIndex val_idx = values_alloc(values, &val);
      *val = (Value){
        .type = type_idx,
        .data = mem,
      };

      map_insert(&machine->inst_map, pc, val_idx);
    } break;
    case IR_cond_br: {
      IrCondBr *cond_br = instruction_extra(chunk, pc);
      Value *cond = ir_ref_value(machine, cond_br->cond);
    } break;
    case IR_block: {} break;
    case IR_loop: {} break;
    case IR_br: {} break;
    case IR_repeat: {} break;
    case IR_ret: {} break;
    case IR_load: {} break;
    case IR_store: {} break;
    case IR_call: {} break;
    case IR_cast_int: {
      IrCastInt *cast_int = instruction_extra(chunk, pc);
      Type *type_dst = types_get(types, cast_int->type);
      Value *val = ir_get_value(machine, cast_int->value);
      Type *type_src = types_get(types, val->type);
      TypeSizeInfo size_info = types_size_info_by_type(types, type_dst);
      void *payload_dst = values_alloc_data(values, size_info.size, size_info.align);
      u32 err = eval_cast_int(type_src->data.integer, val->data, type_dst->data.integer, payload_dst);
      if (err) {
        Todo();
      }
      Value *val_res;
      ValueIndex res = values_alloc(values, &val_res);
      *val_res = (Value){
        .type = cast_int->type,
        .data = payload_dst,
      };

      b32 was_occupied = False;
      InstructionResultMapBucket *bucket = map_insert_key_and_get_bucket(&machine->inst_map, pc, &was_occupied);

      if (was_occupied) {
        // TODO free old value
      }

      bucket->val = res;
    } break;
    }
  }
 
  return IrResult_ok;
}

u32 ir_call_safe(
  IrMachine *machine,
  IrCode *code,
  IrLocation function,
  u32 arg_count,
  ValueIndex *args,
  ValueIndex *result
) {
  IrChunk *chunk = get_chunk(machine->chunks, function.chunk_index);
  IrInstructionIndex instruction_index = function.instruction_index;

  u8 op = opcode(chunk, instruction_index);

  Assert(op == IR_func);

  IrFunc *func = instruction_extra(code, function);

  Assert(arg_count == func->arg_count);

  IrInstructionIndex first_param = instruction_index + 1;

  for (u32 i = 0; i < arg_count; i++) {
    map_insert(
      &machine->inst_map, 
      (IrLocation){ .chunk_index = function.chunk_index, .instruction_index = first_param + i },
      args[i]
    );
  }

  IrCodeIndex pc = first_param + arg_count;

  callstack_append(
    &machine->callstack,
    (IrLocation){}
  );

  return ir_eval_call(machine);
}
