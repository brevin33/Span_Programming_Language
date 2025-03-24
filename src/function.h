#pragma once
#include "utils.h"
#include "types.h"

struct functionPrototype {
    string name;
    vector<Type> paramTypes;
    vector<string> paramNames;
    Type returnType;
    LLVMTypeRef llvmType;
    LLVMValueRef llvmFunction;
    bool variadicArgs;
};
optional<functionPrototype> prototypeFunction(int startPos);
