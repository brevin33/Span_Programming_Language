#include "value.h"
#include "span.h"

Value::Value() {
}

Value::Value(LLVMValueRef llvmValue, const Type& type, Module* module) {
    this->llvmValue = llvmValue;
    this->type = type;
    this->module = module;
}

Value Value::cast(const Type& type) {
    //TODO
    return {};
}

void Value::store(const Value& val) {
    assert(type.isRef());
    LLVMBuildStore(builder, val.llvmValue, llvmValue);
}


void add(Value& lval, Value& rval) {
}

void biopNumberCast(Value& lval, Value& rval) {
}
