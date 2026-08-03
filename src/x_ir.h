#ifndef X
#error "X macro must be defined"
#endif

X(IR_func,          True,  IrFunc,        "func")
X(IR_param,         False, u32,           "param")
X(IR_alloc,         False, u32,           "alloc")
X(IR_condbr,        True,  IrCondBr,      "condbr")
X(IR_block,         False, u32,           "block")
X(IR_loop,          False, u32,           "loop")
X(IR_br,            True,  IrBr,          "br")
X(IR_ret,           False, u32,           "ret")
X(IR_repeat,        False, u32,           "repeat")
X(IR_load,          False, u32,           "load")
X(IR_store,         True,  IrStore,       "store")
X(IR_call,          True,  IrCall,        "call")
X(IR_declaration,   True,  IrDeclaration, "declaration")
X(IR_lookup_typeof, False, u32,           "lookup_typeof")
X(IR_lookup_value,  False, u32,           "lookup_value")
X(IR_as,            True,  IrAs,          "as")
X(IR_unify,         True,  IrUnify,       "unify")
X(IR_type,          True,  IrType,        "type")
X(IR_return_type,   False, u32,           "return_type")
X(IR_param_type,    True,  IrParamType,   "param_type")
