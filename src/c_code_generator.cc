#include "blu.hh"

template<typename Writer>
struct CodeGenerator {
  Writer writer;

  MessageManager *messages = nullptr;
  Source *source = nullptr;

  //Str generate_declaration_name(Declaration decl);
  void output_declaration(Declaration decl);
  void output_definition(Declaration decl);

  void output_source();
};

template<typename Writer>
void CodeGenerator::output_declaration(Declaration decl) {
  Str name = source->get_token_str(decl.name);
  auto typeof_decl = values->get(decl.type);
} 

template<typename Writer>
void CodeGenerator::output_source() {
  auto nodes = source->nodes;

  auto items = nodes->data({0}).root.items;

  for (usize i = 0; i < items.len(); i += 1) {
    auto decl = nodes->data(items.at(i)).declaration;
    output_declaration(decl);
  }

  for (usize i = 0; i < items.len(); i += 1) {
    auto decl = nodes->data(items.at(i)).declaration;
    output_definition(decl);
  }
}

b32 generate_c_code() {
  CodeGenerator gen;
  gen.output_source();
}
