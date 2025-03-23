#include "variable.h"
#include "span.h"

Variable::Variable() {
}

Variable::Variable(string& name, Type& type, Module* module) {
    this->name = name;
    this->module = module;
    LLVMValueRef stackVal = LLVMBuildAlloca(builder, type.llvmType, name.c_str());
    this->value = Value(stackVal, type.ref(), module);
}

Variable::Variable(string& name, Type& type, Value& val, Module* module) {
    this->name = name;
    this->module = module;
    if (type.isRef()) {
        this->value = val;
        return;
    }
    LLVMValueRef stackVal = LLVMBuildAlloca(builder, type.llvmType, name.c_str());
    this->value = Value(stackVal, type.ref(), module);
    store(val);
}

void Variable::store(const Value& val) {
    value.store(val);
}
