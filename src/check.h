#ifndef CHECK_H
#define CHECK_H

#include "blu.h"

typedef struct SourceFile {
  String   filename;
  String   text;
  Tokens   tokens;
  AstNodes ast;
} SourceFile;

typedef struct {
  Messages       *messages;
  StringInterner *strings;
  TypeInterner   *types;
  EnvAllocator   *envs;
  ValueStore     *values;

  u32         source_count;
  SourceFile *sources;

  Env *env_root;

  TypeIndex ty_i8;
  TypeIndex ty_i16;
  TypeIndex ty_i32;
  TypeIndex ty_i64;
  TypeIndex ty_u8;
  TypeIndex ty_u16;
  TypeIndex ty_u32;
  TypeIndex ty_u64;
  TypeIndex ty_type;
} Checker;

typedef struct {
  Messages *messages;
  StringInterner *strings;
  TypeInterner *types;
  EnvAllocator *envs;
  ValueStore *values;

  u32 source_count;
  SourceFile *sources;
} CheckerOptions;

void checker_init(Checker *checker, CheckerOptions *options);
void checker_deinit(Checker *checker);

b32 check_code(Checker *checker);

#endif // CHECK_H
