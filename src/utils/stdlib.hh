#pragma once

#include "toteload.hh"

extern Allocator stdlib_alloc;

// memory is allocated with `malloc`
Str read_file(Str filename);
void write_file(Str filename, Str text);
