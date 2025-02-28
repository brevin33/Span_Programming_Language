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

    optional<Value> implCast(Type& type);

    void store(const Value& val);

    optional<Value> negate();

    Value refToVal();

    Value actualValue();

    Value structVal(int i);

    optional<Value> toBool();

    optional<Value> variadicCast();

public:
    LLVMValueRef llvmValue;
    Type type;
    Module* module;
    bool constant;

private:
private:
};

optional<Value> add(Value& lval, Value& rval);
optional<Value> mul(Value& lval, Value& rval);
optional<Value> div(Value& lval, Value& rval);
optional<Value> sub(Value& lval, Value& rval);
optional<Value> equal(Value& lval, Value& rval);
optional<Value> lessThanOrEqual(Value& lval, Value& rval);
optional<Value> greaterThanOrEqual(Value& lval, Value& rval);
optional<Value> lessThan(Value& lval, Value& rval);
optional<Value> greaterThan(Value& lval, Value& rval);
optional<Value> notEqual(Value& lval, Value& rval);
Value as(Value& lval, Type& rval);
optional<Value> to(Value& lval, Type& rval);
optional<Value> and (Value & lval, Value& rval);
optional<Value> or (Value & lval, Value& rval);
