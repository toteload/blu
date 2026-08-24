#ifndef X
#error "X macro must be defined"
#endif

X(IIR_func,          "func",    False, u32)
X(IIR_param,         "param",   False, u32)
X(IIR_alloc,         "alloc",   False, u32)
X(IIR_load,          "load",    False, u32)
X(IIR_store,         "store",   True,  IIrStore)
X(IIR_block,         "block",   False, u32)
X(IIR_loop,          "loop",    False, u32)
X(IIR_condbr,        "condbr",  True,  IIrCondbr)
X(IIR_br,            "br",      True,  IIrBr)
X(IIR_repeat,        "repeat",  False, u32)
X(IIR_ret,           "ret",     False, u32)
X(IIR_call,          "call",    True,  IIrCall)
X(IIR_int_add,       "int_add", True,  IIrBinary)
X(IIR_int_sub,       "int_sub", True,  IIrBinary)
X(IIR_int_mul,       "int_mul", True,  IIrBinary)
X(IIR_int_div,       "int_div", True,  IIrBinary)
X(IIR_builtin_debug, "#debug",  False, u32)
