#pragma once
#include "utils.h"
#include "type.h"

class Module;

class Value {
public:
    Value();

    Value(LLVMValueRef llvmValue, const Type& type, Module* module);

    Value cast(const Type& type);

    void store(const Value& val);

public:
    LLVMValueRef llvmValue;
    Type type;
    Module* module;

private:
private:
};

void add(Value& lval, Value& rval);

void biopNumberCast(Value& lval, Value& rval);
