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

internal always_inline IrCodeIndex ref_to_code_index(IrRef ref) {
  return ref;
}

internal always_inline IrRef code_index_to_ref(IrCodeIndex idx) {
  return idx;
}

internal always_inline b32 ref_is_value_index(IrRef ref) {
  return (ref & Bitmask_ir_ref_is_value_index) != 0;
}

u8 ir_code_opcode(IrCode *code, IrCodeIndex idx) {
  return code->opcodes[idx];
}

void *ir_code_data(IrCode *code, IrCodeIndex idx) {
  return ptr_offset(code->extra, code->data[idx]);
}

Value *ir_get_value(IrMachine *machine, IrRef ref) {
  if (ref_is_value_index(ref)) {
    return values_get(machine->values, ref_to_value_index(ref));
  }

  return map_find(&machine->inst_map, ref_to_code_index(ref));
}

u32 ir_call_safe(
  IrMachine *machine,
  IrCode *code,
  IrRef function,
  u32 arg_count,
  ValueIndex *args,
  ValueIndex *result
) {
  u8 op = ir_code_opcode(code, ref_to_code_index(function));

  Assert(op == IR_call);

  IrFunc *func = ir_code_data(code, function);

  Assert(arg_count == func->arg_count);

  IrCodeIndex first_param = ref_to_code_index(function) + 1;

  for (u32 i = 0; i < arg_count; i++) {
    map_insert(&machine->inst_map, first_param + i, args[i]);
  }

  IrCodeIndex pc = first_param + arg_count;

  ValueStore *values = machine->values;

  while (True) {
    op = ir_code_opcode(code, pc);

    switch (op) {
    case IR_cast_int: {
      IrCastInt *cast_int = ir_code_data(code, pc);
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
}
