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

void Variable::store(const Value& val) {
    value.store(val);
}
