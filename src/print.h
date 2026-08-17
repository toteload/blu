#ifndef PRINT_H
#define PRINT_H

#include "ir.h"
#include "compiler.h"
#include "tokens.h"
#include "ast.h"

void ir_chunk_print(FILE *out, Compiler *compiler, IrChunk *chunk);
void type_index_print(FILE *out, TypeInterner *types, TypeIndex idx);
void value_print(FILE *out, Compiler *compiler, ValueIndex idx);
void print_tokens(Tokens *tokens, String text);
void print_ast_nodes(AstNodes *nodes, Tokens *tokens, String text);

#endif // PRINT_H
