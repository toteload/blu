#pragma once

struct Interpreter {
  StringInterner *strings;
  TypeInterner *types;
  ValueStore *values;

  void init(StringInterner *strings, TypeInterner *types, ValueStore *values);
  void deinit();

  void run(Source *source);
};
