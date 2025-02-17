#include "function.h"


Function::Function() {
}

Function::Function(const Type& returnType, const string& name, const vector<Type>& paramTypes, const vector<string> paramNames, Module* module) {
    this->returnType = returnType;
    this->name = name;
    this->paramNames = paramNames;
    this->paramTypes = paramTypes;
    this->module = module;

    LLVMTypeRef llvmTypes[256];
    for (int i = 0; i < paramNames.size(); i++) {
        llvmTypes[i] = paramTypes[i].llvmType;
    }
    this->llvmType = LLVMFunctionType(returnType.llvmType, llvmTypes, paramTypes.size(), 0);
    this->llvmValue = LLVMAddFunction(module->llvmModule, name.c_str(), this->llvmType);
}

Value Function::getParamValue(int paramNumber) {
    return Value(LLVMGetParam(llvmValue, paramNumber), paramTypes[paramNumber], module);
}
