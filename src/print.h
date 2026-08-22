#ifndef PRINT_H
#define PRINT_H

#include "ir.h"
#include "tokens.h"
#include "ast.h"
#include "compiler.h"

void print_sir_chunk(FILE *out, Compiler *compiler, SIrChunk *chunk);
void print_iir_chunk(FILE *out, Compiler *compiler, IIrChunk *chunk);

#define PrintFlag_expand_function (1 << 0)

void print_value_raw(FILE *out, Compiler *compiler, u32 flags, TypeIndex type, void *data);

void print_type(FILE *out, TypeInterner *types, TypeIndex idx);
void print_tokens(Tokens *tokens, String text);
void print_ast_nodes(AstNodes *nodes, Tokens *tokens, String text);

#endif // PRINT_H
