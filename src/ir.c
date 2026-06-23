#include "ir.h"

#define Bitmask_ir_ref_is_value_index (Cast(u32, 1) << 31)

u32 instruction_index_hash(void *context, InstructionIndex idx) {
  Unused(context);

  return idx;
}

b32 instruction_index_eq(void *context, InstructionIndex a, InstructionIndex b) {
  Unused(context);

  return a == b;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#define HASHMAP_NAME            InstructionResultMap
#define HASHMAP_FUNCTION_PREFIX map
#define HASHMAP_KEY_TYPE        InstructionIndex
#define HASHMAP_VALUE_TYPE      ValueIndex
#define HASHMAP_HASH_FN         instruction_index_hash
#define HASHMAP_KEY_COMPARE_FN  instruction_index_eq
#define HASHMAP_LINKAGE         internal
#define HASHMAP_OUTPUT_DEFINITIONS
#include "hashmap.h"
#pragma clang diagnostic pop

#define SEGMENTLIST_NAME            CallStack
#define SEGMENTLIST_TYPE            CallFrame
#define SEGMENTLIST_MIN_SIZE_LOG2   5
#define SEGMENTLIST_SEGMENT_COUNT   24
#define SEGMENTLIST_FUNCTION_PREFIX callstack
#define SEGMENTLIST_LINKAGE         internal
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

internal always_inline InstructionIndex ref_to_instruction_index(IrRef ref) {
  return ref;
}

internal always_inline IrRef instruction_index_to_ref(InstructionIndex idx) {
  return idx;
}

internal always_inline ValueIndex ref_to_value_index(IrRef ref) {
  return ref & ~Bitmask_ir_ref_is_value_index;
}

internal always_inline b32 ref_is_value_index(IrRef ref) {
  return (ref & Bitmask_ir_ref_is_value_index) != 0;
}

u8 opcode(IrChunk *chunk, InstructionIndex idx) {
  return chunk->opcodes[idx];
}

u32 instruction_data(IrChunk *chunk, InstructionIndex idx) {
  return chunk->data[idx];
}

void *instruction_extra(IrChunk *chunk, InstructionIndex idx) {
  return ptr_offset(chunk->extra, chunk->data[idx]);
}

ValueIndex ref_value_index(IrMachine *machine, IrRef ref) {
  if (ref_is_value_index(ref)) {
    return ref_to_value_index(ref);
  }

  return map_find(&machine->inst_map, ref_to_instruction_index(ref));
}

internal void clear_block_values(IrMachine *machine, CallFrame *frame, InstructionIndex block) {
  ValueStack *stack = &frame->value_stack;

  for (u32 i = stack->len; i-- > 0; ) {
    ValueStackElement e = *value_stack_ptr_at_unchecked(stack, i);
    if (e.kind == ValueStackElement_block_marker && e.loc == block) {
      stack->len = i;
      break;
    }

    Assert(e.kind == ValueStackElement_value);

    ValueIndex val = *map_find(&machine->inst_map, e.loc);

    values_dealloc(machine->values, val);
  }
}

internal void clear_frame_values(IrMachine *machine, CallFrame *frame) {
  ValueStack *stack = &frame->value_stack;

  for (u32 i = stack->len; i-- > 0; ) {
    ValueStackElement e = *value_stack_ptr_at_unchecked(stack, i);
    if (e.kind == ValueStackElement_block_marker) {
      continue;
    }

    Assert(e.kind == ValueStackElement_value);

    ValueIndex val = *map_find(&machine->inst_map, e.loc);

    values_dealloc(machine->values, val);
  }
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
  InstructionIndex instruction_index = function.instruction_index;

  u8 op = opcode(chunk, instruction_index);

  Assert(op == IR_func);

  IrFunc *func = instruction_extra(code, function);

  Assert(arg_count == func->arg_count);

  InstructionIndex first_param = instruction_index + 1;

  for (u32 i = 0; i < arg_count; i++) {
    map_insert(
      &machine->inst_map, 
      (IrLocation){ .chunk_index = function.chunk_index, .instruction_index = first_param + i },
      args[i]
    );
  }

  InstructionIndex pc = first_param + arg_count;

  callstack_append(
    &machine->callstack,
    (IrLocation){}
  );
  
  IrLocation pc = *callstack_ptr_at_unchecked(&machine->callstack, machine->callstack.len-1);
  IrChunk *chunk = get_chunk(machine->chunks, pc.chunk_index);

  while (True) {
    op = opcode(chunk, pc.instruction_index);

    switch (op) {
    case IR_alloc: {
      TypeIndex type_idx = instruction_data(chunk, pc.instruction_index);
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
      value_stack_append(&machine->value_stack, arena, (ValueStackElement){ .kind = ValueStackElement_value, .loc = pc });
    } break;
    case IR_cond_br: {
      IrCondBr *cond_br = instruction_extra(chunk, pc.instruction_index);
      Value *cond = ir_ref_value(machine, cond_br->cond);

      // A cond_br can only jump forward into a block, and not backwards to break out of an enclosing block.
      // This way you don't have to branch on whether this is a forwards or backwards jump.
      IrInstructionIndex target = (*Cast(u8*, cond->data)) ? cond->then : cond->otherwise;
      pc.instruction_index = target;
    } break;
    case IR_block: {
      value_stack_append(&machine->value_stack, arena, (ValueStackElement){ .kind = ValueStackElement_block_marker, .loc = pc });
    } break;
    case IR_loop: {
      value_stack_append(&machine->value_stack, arena, (ValueStackElement){ .kind = ValueStackElement_block_marker, .loc = pc });
    } break;
    case IR_br: {
      InstructionIndex block_index = instruction_data(chunk, pc.instruction_index);
      clear_block_values(machine, block_index);
      u32 block_size = instruction_data(chunk, block_index);
      pc.instruction_index = block_index + block_size;
    } break;
    case IR_repeat: {
      InstructionIndex block_index = instruction_data(chunk, pc.instruction_index);
      clear_block_values(machine, block_index);
      pc.instruction_index = block_index;
    } break;
    case IR_ret: {
      clear_function_values(machine);
      IrRef ref = instruction_data(chunk, pc.instruction_index);
      machine->return_value = ref_to_value_index(ref);
      return EvalResult_ok;
    } break;
    case IR_load: {
      Panic();
    } break;
    case IR_store: {
      IrStore *store = instruction_extra(chunk, pc.instruction_index);
      ValueIndex val_idx_from = ref_to_value_index(store->value);
      ValueIndex val_idx_target = ref_to_value_index(store->dst);
      Value *from = values_get(machine->values, val_idx_from);
      Value *target = values_get(machine->values, val_idx_target);
      memcpy(target->data, from->data, target->data_size);
    } break;
    case IR_call: {
      Panic();
    } break;
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

      map_insert(&machine->inst_map, pc, res);
    } break;
    }

    pc.instruction_index++;
  }
 
  return IrResult_ok;

}
