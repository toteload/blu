#include "blu.hh"

void Interpreter::init(StringInterner *strings, TypeInterner *types, ValueStore *values) {
  this->strings = strings;
  this->types   = types;
  this->values  = values;
}

void Interpreter::run(Source *source) {}
