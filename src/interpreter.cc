#include "blu.hh"

void Interpreter::init(StringInterner *strings, TypeInterner *types, ValueStore *values) {
  this->strings = strings;
  this->types   = types;
  this->values  = values;
}

void Interpreter::run(Source *source) {
  // 1. Get all declarations in root and add them to env.
  // 2. Call main.
}
