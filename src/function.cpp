#include "function.h"
#include "span.h"


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

Value Function::call(vector<Value> vals) {
    vector<LLVMValueRef> llvmVals;
    for (int j = 0; j < vals.size(); j++) {
        llvmVals.push_back(vals[j].llvmValue);
    }

    Value val;
    if (returnType.name != "void") {
        val.llvmValue = LLVMBuildCall2(builder, llvmType, llvmValue, llvmVals.data(), llvmVals.size(), name.c_str());
    } else {
        val.llvmValue = LLVMBuildCall2(builder, llvmType, llvmValue, llvmVals.data(), llvmVals.size(), "");
    }
    val.type = returnType;
    return val;
}
