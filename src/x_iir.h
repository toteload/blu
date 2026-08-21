#ifndef X
#error "X macro must be defined"
#endif

X(IIR_func,          "func",    True,  IFunc)
X(IIR_param,         "param",   False, u32)
X(IIR_alloc,         "alloc",   False, u32)
X(IIR_load,          "load",    True,  ILoad)
X(IIR_store,         "store",   True,  IStore)
X(IIR_block,         "block",   True,  IBlock)
X(IIR_loop,          "loop",    False, u32)
X(IIR_condbr,        "condbr",  True,  ICondbr)
X(IIR_br,            "br",      True,  IBr)
X(IIR_repeat,        "repeat",  False, u32)
X(IIR_ret,           "ret",     True,  IRet)
X(IIR_call,          "call",    True,  ICall)
X(IIR_builtin_debug, "#debug",  True,  IBuiltinDebug)
