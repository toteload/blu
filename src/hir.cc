#include "hir.hh"

struct HirGenerator {
  Source *source = nullptr;
};

//  0 hir_declaration
//      - type = index into type interner?
//      - value = index into hir code
//  1 hir_function
//      - return_type = ???
//      - parameter_count = number of parameters of function
//      - body_length = number of instructions in body
//  2 hir_param
//      - type = ???
//  3 hir_const_int
//      - value = ???
//  4 hir_cmp_less_equal %2, %3
//  5 hir_cond_br %4, %6, %8 
//  6 hir_const_int
//      - value = ???
//  7 hir_return %6
//  8 hir_const_int
//      - value = where to store a value
//  9 hir_sub %2, %8
// 10 hir_call %1, %9
// 11 hir_mul %2, %10
// 12 hir_return %11

b32 generate_hir(Source *source, HirCode *code) {
  Debug_assert(source->nodes->kind(0) == Ast_root);

  auto root = source->nodes->data(0).root;
  for (usize i = 0; i < root.items.len(); i++) {

  }
}
