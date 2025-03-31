#pragma once
#include "utils.h"
#include "types.h"
#include "template.h"

struct functionPrototype {
    string name;
    vector<Type> paramTypes;
    vector<string> paramNames;
    optional<templateNames> templates;
    Type returnType;
    optional<Type> methodType;
    LLVMTypeRef llvmType;
    LLVMValueRef llvmFunction;
    bool variadicArgs;
    int startPosition;
    bool isReal();
};
optional<functionPrototype> prototypeFunction(int startPos);
