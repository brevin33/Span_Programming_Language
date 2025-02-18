#pragma once
#include "utils.h"
#include "type.h"

class Module;

class Value {
public:
    Value();

    Value(LLVMValueRef llvmValue, const Type& type, Module* module, bool constant = false);

    Value(u64 num, Module* module);

    Value(i64 num, Module* module);

    Value(double num, Module* module);

    optional<Value> cast(Type& type);

    void store(const Value& val);

    optional<Value> negate();

    Value refToVal();

    Value actualValue();

public:
    LLVMValueRef llvmValue;
    Type type;
    Module* module;
    bool constant;

private:
private:
};

optional<Value> add(Value& lval, Value& rval);
