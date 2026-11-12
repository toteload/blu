#ifndef BYTECODE_H
#define BYTECODE_H

#include "blu.h"
#include "types.h"
#include "ir.h"

typedef struct { u32 x; } BcOperand;

#define BC_OPERAND_CONSTANT_BIT (Cast(u32, 1) << 31)

always_inline b32 bc_operand_is_constant(BcOperand o) {
  return (o.x & BC_OPERAND_CONSTANT_BIT) != 0;
}

always_inline u32 bc_operand_offset(BcOperand o) {
  return o.x & ~BC_OPERAND_CONSTANT_BIT;
}

always_inline BcOperand bc_operand_frame(u32 offset) {
  ASSERT_SAFE((offset & BC_OPERAND_CONSTANT_BIT) == 0);
  return (BcOperand){offset};
}

always_inline BcOperand bc_operand_constant(u32 offset) {
  ASSERT_SAFE((offset & BC_OPERAND_CONSTANT_BIT) == 0);
  return (BcOperand){offset | BC_OPERAND_CONSTANT_BIT};
}

typedef enum {
#define X(k, _1, _2) k,
#include "x_bytecode.h"
#undef X
} BcOpcode;

typedef struct {
  u32 op;
  BcOperand dst;
  BcOperand src;
  u32 size;
} BcCopy;

typedef struct {
  u32 op;
  u32 target;
} BcJmp;

typedef struct {
  u32 op;
  BcOperand cond;
  u32 target;
} BcJmpIfFalse;

typedef struct {
  u32 op;
  BcOperand dst;
  BcOperand ptr;
  u32 size;
} BcLoad;

typedef struct {
  u32 op;
  BcOperand ptr;
  BcOperand src;
  u32 size;
} BcStore;

typedef struct {
  u32 op;
  BcOperand dst;
  BcOperand func;
  u32 arg_count;
  BcOperand args[];
} BcCall;

typedef struct {
  u32 op;
} BcRet;

typedef struct {
  u32 op;
  BcOperand dst;
  BcOperand src;
  TypeInteger from;
  TypeInteger to;
} BcIntCast;

typedef struct {
  u32 op;
  BcOperand dst;
  BcOperand lhs;
  BcOperand rhs;
  TypeInteger type;
} BcIntBinary;

typedef struct {
  u32 op;
  BcOperand dst;
  BcOperand src;
  u32 size;
} BcUnary;

typedef struct {
  u32 op;
  BcOperand dst;
  BcOperand lhs;
  BcOperand rhs;
  u32 size;
} BcBinary;

typedef struct {
  u32 op;
  BcOperand dst;
  BcOperand src;
} BcAddr;

typedef struct {
  u32 op;
  BcOperand index;
  BcOperand len;
} BcBoundsCheck;

typedef struct {
  u32 op;
  BcOperand dst;
  BcOperand ptr;
  BcOperand index;
  u32 stride;
} BcElemPtr;

typedef struct {
  u32 op;
  BcOperand dst;
  BcOperand src;
  TypeKey type;
  u32 size;
} BcDebug;

typedef struct {
  usize len;
  void *data;
} BcSlice;

#endif
