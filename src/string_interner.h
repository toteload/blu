#ifndef STRING_INTERNER_H
#define STRING_INTERNER_H

#include "blu.h"

#define INTERNER_NAME            StringInterner
#define INTERNER_TYPE            String
#define INTERNER_INDEX_TYPE      StringKey
#define INTERNER_FUNCTION_PREFIX strings
#define INTERNER_OUTPUT_TYPES
#define INTERNER_OUTPUT_DECLARATIONS
#include "interner.h"

#endif // STRING_INTERNER_H
