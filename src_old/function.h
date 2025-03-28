#pragma once
#include "utils.h"

#include "value.h"
#include "module.h"
#include "scope.h"

struct Module;

class Function {
public:
    Function();

    Function(const Type& returnType, const string& name, const vector<Type>& paramTypes, const vector<string> paramNames, Module* module, bool variadic, bool external, optional<Type> methodOfType,
        const vector<Type>& templateTypes, const vector<string>& templateNames);

    Value getParamValue(int paramNumber);

    Value call(vector<Value> vals, Module* module);



public:
    Type returnType;
    string name;
    vector<Type> paramTypes;
    vector<string> paramNames;
    Scope scope;
    LLVMTypeRef llvmType;
    LLVMValueRef llvmValue;
    LLVMBasicBlockRef entry;
    bool variadic;
    bool external;
    std::unordered_map<u64, LLVMValueRef> moduleToFunc;
    optional<Type> methodOfType;
    vector<string> templateNames;
    vector<Type> templateTypes;
    vector<Function> templateFunctions;

private:
    Module* module;

private:
};