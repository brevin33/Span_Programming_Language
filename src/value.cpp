#include "value.h"
#include "span.h"

Value::Value() {
}

Value::Value(LLVMValueRef llvmValue, const Type& type, Module* module, bool constant) {
    this->llvmValue = llvmValue;
    this->type = type;
    this->module = module;
    this->constant = constant;
}

Value::Value(u64 num, Module* module) {
    this->type = nameToType["u64"].front();
    this->llvmValue = LLVMConstInt(type.llvmType, num, 0);
    this->constant = true;
    this->module = module;
}

Value::Value(i64 num, Module* module) {
    this->type = nameToType["i64"].front();
    this->llvmValue = LLVMConstInt(type.llvmType, num, 1);
    this->constant = true;
    this->module = module;
}

Value::Value(double num, Module* module) {
    this->type = nameToType["f64"].front();
    this->llvmValue = LLVMConstReal(type.llvmType, num);
    this->constant = true;
    this->module = module;
}

optional<Value> Value::implCast(Type& type) {
    if (this->type == type) return *this;
    Value v = actualValue();
    if (type == v.type) return v;
    if (v.type.isUInt()) {
        if (type.isUInt()) {
            u64 vWidth = v.type.getNumberWidth();
            u64 tWidth = type.getNumberWidth();
            if (vWidth > tWidth) {
                v.llvmValue = LLVMBuildTrunc(builder, v.llvmValue, type.llvmType, "castUint");
            } else if (tWidth > vWidth) {
                v.llvmValue = LLVMBuildZExt(builder, v.llvmValue, type.llvmType, "castUint");
            }
            v.type = type;
            return v;
        } else if (type.isInt()) {
            u64 vWidth = v.type.getNumberWidth();
            u64 tWidth = type.getNumberWidth();
            if (vWidth > tWidth) {
                v.llvmValue = LLVMBuildTrunc(builder, v.llvmValue, type.llvmType, "castUint");
            } else if (tWidth > vWidth) {
                v.llvmValue = LLVMBuildSExt(builder, v.llvmValue, type.llvmType, "castUint");
            }
            v.type = type;
            return v;
        } else if (type.isFloat()) {
            v.llvmValue = LLVMBuildUIToFP(builder, v.llvmValue, type.llvmType, "castUint");
            v.type = type;
            return v;
        }
    } else if (v.type.isInt()) {
        if (type.isUInt()) {
            u64 vWidth = v.type.getNumberWidth();
            u64 tWidth = type.getNumberWidth();
            if (vWidth > tWidth) {
                v.llvmValue = LLVMBuildTrunc(builder, v.llvmValue, type.llvmType, "castint");
            } else if (tWidth > vWidth) {
                v.llvmValue = LLVMBuildZExt(builder, v.llvmValue, type.llvmType, "castint");
            }
            v.type = type;
            return v;
        } else if (type.isInt()) {
            u64 vWidth = v.type.getNumberWidth();
            u64 tWidth = type.getNumberWidth();
            if (vWidth > tWidth) {
                v.llvmValue = LLVMBuildTrunc(builder, v.llvmValue, type.llvmType, "castint");
            } else if (tWidth > vWidth) {
                v.llvmValue = LLVMBuildSExt(builder, v.llvmValue, type.llvmType, "castint");
            }
            v.type = type;
            return v;
        } else if (type.isFloat()) {
            v.llvmValue = LLVMBuildSIToFP(builder, v.llvmValue, type.llvmType, "castint");
            v.type = type;
            return v;
        }
    } else if (v.type.isFloat()) {
        if (type.isUInt()) {
            v.llvmValue = LLVMBuildFPToUI(builder, v.llvmValue, type.llvmType, "castfloat");
            v.type = type;
            return v;
        } else if (type.isInt()) {
            v.llvmValue = LLVMBuildFPToSI(builder, v.llvmValue, type.llvmType, "castfloat");
            v.type = type;
            return v;
        } else if (type.isFloat()) {
            v.llvmValue = LLVMBuildFPCast(builder, v.llvmValue, type.llvmType, "castfloat");
            v.type = type;
            return v;
        }
    }
    return nullopt;
}

optional<Value> Value::cast(Type& type) {
    Value v = actualValue();
    optional<Value> implValue = implCast(type);
    //TODO: cast overloading
    return implValue;
}

void Value::store(const Value& val) {
    assert(type.isRef());
    LLVMBuildStore(builder, val.llvmValue, llvmValue);
}

optional<Value> Value::negate() {
    Value v = actualValue();
    if (v.type.isUInt()) {
        v = v.cast(nameToType["i" + to_string(v.type.getNumberWidth())].front()).value();
        v.llvmValue = LLVMBuildNeg(builder, v.llvmValue, "neg");
        return v;
    } else if (v.type.isInt()) {
        v.llvmValue = LLVMBuildNeg(builder, v.llvmValue, "neg");
        return v;
    } else if (v.type.isFloat()) {
        v.llvmValue = LLVMBuildFNeg(builder, v.llvmValue, "neg");
        return v;
    } else {
        //TODO: operator overload
        return nullopt;
    }
}

Value Value::refToVal() {
    assert(type.isRef());
    Value v = *this;
    v.type = v.type.dereference();
    v.llvmValue = LLVMBuildLoad2(builder, v.type.llvmType, v.llvmValue, "loadRef");
    return v;
}

Value Value::actualValue() {
    Value v = *this;
    while (v.type.isRef()) {
        v = refToVal();
    }
    return v;
}

optional<Value> Value::toBool() {
    Value zero = Value(LLVMConstInt(LLVMInt32Type(), 0, 0), nameToType["i32"].front(), module, true);
    optional<Value> zeroAsVal = zero.cast(type);
    if (!zeroAsVal.has_value() || (!zeroAsVal.value().type.isNumber())) {
        return nullopt;
    }
    LLVMValueRef condition;
    if (zeroAsVal.value().type.isFloat()) {
        condition = LLVMBuildFCmp(builder, LLVMRealONE, zeroAsVal.value().llvmValue, llvmValue, "cmp");
    } else {
        condition = LLVMBuildICmp(builder, LLVMIntNE, zeroAsVal.value().llvmValue, llvmValue, "cmp");
    }
    return Value(condition, nameToType["i1"].front(), module, constant);
}

optional<Value> Value::variadicCast() {
    Value v;
    if (type.isRef()) v = actualValue();
    else
        v = *this;
    if (v.type.isFloat()) {
        v.llvmValue = LLVMBuildFPCast(builder, v.llvmValue, LLVMDoubleType(), "upcastVaradicArg");
        v.type = nameToType["f64"].front();
        return v;
    }
    if (v.type.isInt()) {
        u64 intwidth = v.type.getNumberWidth();
        if (intwidth > 64) return nullopt;
        if (intwidth < 32) {
            v.llvmValue = LLVMBuildSExt(builder, v.llvmValue, LLVMInt32Type(), "upcastVaradicArg");
            v.type = nameToType["i32"].front();
        }
        return v;
    }
    if (v.type.isUInt()) {
        u64 intwidth = v.type.getNumberWidth();
        if (intwidth > 64) return nullopt;
        if (intwidth < 32) {
            v.llvmValue = LLVMBuildZExt(builder, v.llvmValue, LLVMInt32Type(), "upcastVaradicArg");
            v.type = nameToType["u32"].front();
        }
        return v;
    }
    //TODO: passing small structs
    return nullopt;
}

void castNumberForBiop(Value& lval, Value& rval) {
    int lwidth = lval.type.getNumberWidth();
    int rwidth = rval.type.getNumberWidth();

    if (lval.type.isFloat() && !rval.type.isFloat()) {
        if (rval.type.isInt()) {
            rval.type = lval.type;
            rval.llvmValue = LLVMBuildSIToFP(builder, rval.llvmValue, lval.type.llvmType, "intToFloat");
            return;
        } else {
            rval.type = lval.type;
            rval.llvmValue = LLVMBuildUIToFP(builder, rval.llvmValue, lval.type.llvmType, "intToFloat");
            return;
        }
    } else if (!lval.type.isFloat() && rval.type.isFloat()) {
        if (lval.type.isInt()) {
            lval.type = rval.type;
            lval.llvmValue = LLVMBuildSIToFP(builder, lval.llvmValue, rval.type.llvmType, "intToFloat");
            return;
        } else {
            lval.type = rval.type;
            lval.llvmValue = LLVMBuildUIToFP(builder, lval.llvmValue, rval.type.llvmType, "intToFloat");
            return;
        }
    }

    if (lval.type.isFloat() && rval.type.isFloat()) {
        if (lval.constant) {
            lval.type = rval.type;
            lval.llvmValue = LLVMBuildFPCast(builder, lval.llvmValue, rval.type.llvmType, "floatTofloat");
            return;
        } else if (rval.constant) {
            rval.type = lval.type;
            rval.llvmValue = LLVMBuildFPCast(builder, rval.llvmValue, lval.type.llvmType, "floatTofloat");
            return;
        } else {
            if (lwidth < rwidth) {
                rval.type = lval.type;
                rval.llvmValue = LLVMBuildFPCast(builder, rval.llvmValue, lval.type.llvmType, "floatTofloat");
            } else {
                lval.type = rval.type;
                lval.llvmValue = LLVMBuildFPCast(builder, lval.llvmValue, rval.type.llvmType, "floatTofloat");
            }
            return;
        }
    }



    if (lwidth > rwidth && lval.constant == rval.constant) {
        if (lval.type.isInt() || rval.type.isInt()) {
            lval.type.name[0] = 'i';
            rval.type = lval.type;
            rval.llvmValue = LLVMBuildSExt(builder, rval.llvmValue, lval.type.llvmType, "intToint");
            return;
        } else {
            rval.type = lval.type;
            rval.llvmValue = LLVMBuildZExt(builder, rval.llvmValue, lval.type.llvmType, "intToint");
            return;
        }
    } else if (lval.constant == rval.constant) {
        if (lval.type.isInt() || rval.type.isInt()) {
            rval.type.name[0] = 'i';
            lval.type = rval.type;
            lval.llvmValue = LLVMBuildSExt(builder, lval.llvmValue, rval.type.llvmType, "intToint");
            return;
        } else {
            lval.type = rval.type;
            lval.llvmValue = LLVMBuildZExt(builder, lval.llvmValue, rval.type.llvmType, "intToint");
            return;
        }
    }

    if (lval.constant) {
        if (lwidth > rwidth) {
            if (lval.type.isInt()) {
                rval.type.name[0] = 'i';
            }
            lval.type = rval.type;
            lval.llvmValue = LLVMBuildTrunc(builder, lval.llvmValue, rval.type.llvmType, "intToint");
        } else {
            if (lval.type.isInt()) {
                rval.type.name[0] = 'i';
                lval.llvmValue = LLVMBuildSExt(builder, lval.llvmValue, rval.type.llvmType, "intToint");
            } else {
                lval.llvmValue = LLVMBuildZExt(builder, lval.llvmValue, rval.type.llvmType, "intToint");
            }
            lval.type = rval.type;
            return;
        }
    } else {
        if (rwidth > lwidth) {
            if (rval.type.isInt()) {
                lval.type.name[0] = 'i';
            }
            rval.type = lval.type;
            rval.llvmValue = LLVMBuildTrunc(builder, rval.llvmValue, lval.type.llvmType, "intToint");
        } else {
            if (rval.type.isInt()) {
                lval.type.name[0] = 'i';
                rval.llvmValue = LLVMBuildSExt(builder, rval.llvmValue, lval.type.llvmType, "intToint");
            } else {
                rval.llvmValue = LLVMBuildZExt(builder, rval.llvmValue, lval.type.llvmType, "intToint");
            }
            rval.type = rval.type;
            return;
        }
    }

    return;
}

optional<Value> add(Value& lval, Value& rval) {
    Value l = lval.actualValue();
    Value r = rval.actualValue();
    if (l.type.isNumber() && r.type.isNumber()) {
        castNumberForBiop(l, r);
        LLVMValueRef val;
        if (l.type.isFloat()) {
            val = LLVMBuildFAdd(builder, l.llvmValue, r.llvmValue, "addf");
        } else {
            val = LLVMBuildAdd(builder, l.llvmValue, r.llvmValue, "addf");
        }
        return Value(val, l.type, l.module, l.constant && r.constant);
    }
    vector<Function>& funcs = nameToFunction["add"];
    for (int i = 0; i < funcs.size(); i++) {
        Function& func = funcs[i];
        if (func.paramNames.size() != 2) continue;
        if (func.paramTypes[0] == lval.type && func.paramTypes[1] == rval.type) return func.call({ lval, rval }, activeModule);
    }
    return nullopt;
}

optional<Value> sub(Value& lval, Value& rval) {
    Value l = lval.actualValue();
    Value r = rval.actualValue();
    if (l.type.isNumber() && r.type.isNumber()) {
        castNumberForBiop(l, r);
        LLVMValueRef val;
        if (l.type.isFloat()) {
            val = LLVMBuildFSub(builder, l.llvmValue, r.llvmValue, "subf");
        } else {
            val = LLVMBuildSub(builder, l.llvmValue, r.llvmValue, "subf");
        }
        return Value(val, l.type, l.module, l.constant && r.constant);
    }
    vector<Function>& funcs = nameToFunction["sub"];
    for (int i = 0; i < funcs.size(); i++) {
        Function& func = funcs[i];
        if (func.paramNames.size() != 2) continue;
        if (func.paramTypes[0] == lval.type && func.paramTypes[1] == rval.type) return func.call({ lval, rval }, activeModule);
    }
    return nullopt;
}

optional<Value> equal(Value& lval, Value& rval) {
    Value l = lval.actualValue();
    Value r = rval.actualValue();
    if (l.type.isNumber() && r.type.isNumber()) {
        castNumberForBiop(l, r);
        LLVMValueRef val;
        if (l.type.isFloat()) {
            val = LLVMBuildFCmp(builder, LLVMRealOEQ, l.llvmValue, r.llvmValue, "eq");
        } else {
            val = LLVMBuildICmp(builder, LLVMIntEQ, l.llvmValue, r.llvmValue, "eq");
        }
        return Value(val, nameToType["bool"].front(), l.module, l.constant && r.constant);
    }
    vector<Function>& funcs = nameToFunction["equal"];
    for (int i = 0; i < funcs.size(); i++) {
        Function& func = funcs[i];
        if (func.paramNames.size() != 2) continue;
        if (func.paramTypes[0] == lval.type && func.paramTypes[1] == rval.type) return func.call({ lval, rval }, activeModule);
    }
    return nullopt;
}

optional<Value> lessThanOrEqual(Value& lval, Value& rval) {
    Value l = lval.actualValue();
    Value r = rval.actualValue();
    if (l.type.isNumber() && r.type.isNumber()) {
        castNumberForBiop(l, r);
        LLVMValueRef val;
        if (l.type.isFloat()) {
            val = LLVMBuildFCmp(builder, LLVMRealOLE, l.llvmValue, r.llvmValue, "leq");
        } else {
            if (l.type.isInt()) {
                val = LLVMBuildICmp(builder, LLVMIntSLE, l.llvmValue, r.llvmValue, "leq");
            } else {
                val = LLVMBuildICmp(builder, LLVMIntULE, l.llvmValue, r.llvmValue, "leq");
            }
        }
        return Value(val, nameToType["bool"].front(), l.module, l.constant && r.constant);
    }
    vector<Function>& funcs = nameToFunction["lessThanOrEqual"];
    for (int i = 0; i < funcs.size(); i++) {
        Function& func = funcs[i];
        if (func.paramNames.size() != 2) continue;
        if (func.paramTypes[0] == lval.type && func.paramTypes[1] == rval.type) return func.call({ lval, rval }, activeModule);
    }
    return nullopt;
}

optional<Value> greaterThanOrEqual(Value& lval, Value& rval) {
    Value l = lval.actualValue();
    Value r = rval.actualValue();
    if (l.type.isNumber() && r.type.isNumber()) {
        castNumberForBiop(l, r);
        LLVMValueRef val;
        if (l.type.isFloat()) {
            val = LLVMBuildFCmp(builder, LLVMRealOGE, l.llvmValue, r.llvmValue, "geq");
        } else {
            if (l.type.isInt()) {
                val = LLVMBuildICmp(builder, LLVMIntSGE, l.llvmValue, r.llvmValue, "geq");
            } else {
                val = LLVMBuildICmp(builder, LLVMIntUGE, l.llvmValue, r.llvmValue, "geq");
            }
        }
        return Value(val, nameToType["bool"].front(), l.module, l.constant && r.constant);
    }
    vector<Function>& funcs = nameToFunction["greaterThanOrEqual"];
    for (int i = 0; i < funcs.size(); i++) {
        Function& func = funcs[i];
        if (func.paramNames.size() != 2) continue;
        if (func.paramTypes[0] == lval.type && func.paramTypes[1] == rval.type) return func.call({ lval, rval }, activeModule);
    }
    return nullopt;
}

optional<Value> lessThan(Value& lval, Value& rval) {
    Value l = lval.actualValue();
    Value r = rval.actualValue();
    if (l.type.isNumber() && r.type.isNumber()) {
        castNumberForBiop(l, r);
        LLVMValueRef val;
        if (l.type.isFloat()) {
            val = LLVMBuildFCmp(builder, LLVMRealOLT, l.llvmValue, r.llvmValue, "less");
        } else {
            if (l.type.isInt()) {
                val = LLVMBuildICmp(builder, LLVMIntSLT, l.llvmValue, r.llvmValue, "less");
            } else {
                val = LLVMBuildICmp(builder, LLVMIntULT, l.llvmValue, r.llvmValue, "less");
            }
        }
        return Value(val, nameToType["bool"].front(), l.module, l.constant && r.constant);
    }
    vector<Function>& funcs = nameToFunction["lessThan"];
    for (int i = 0; i < funcs.size(); i++) {
        Function& func = funcs[i];
        if (func.paramNames.size() != 2) continue;
        if (func.paramTypes[0] == lval.type && func.paramTypes[1] == rval.type) return func.call({ lval, rval }, activeModule);
    }
    return nullopt;
}

optional<Value> greaterThan(Value& lval, Value& rval) {
    Value l = lval.actualValue();
    Value r = rval.actualValue();
    if (l.type.isNumber() && r.type.isNumber()) {
        castNumberForBiop(l, r);
        LLVMValueRef val;
        if (l.type.isFloat()) {
            val = LLVMBuildFCmp(builder, LLVMRealOGT, l.llvmValue, r.llvmValue, "gr");
        } else {
            if (l.type.isInt()) {
                val = LLVMBuildICmp(builder, LLVMIntSGT, l.llvmValue, r.llvmValue, "gr");
            } else {
                val = LLVMBuildICmp(builder, LLVMIntUGT, l.llvmValue, r.llvmValue, "gr");
            }
        }
        return Value(val, nameToType["bool"].front(), l.module, l.constant && r.constant);
    }
    vector<Function>& funcs = nameToFunction["greaterThan"];
    for (int i = 0; i < funcs.size(); i++) {
        Function& func = funcs[i];
        if (func.paramNames.size() != 2) continue;
        if (func.paramTypes[0] == lval.type && func.paramTypes[1] == rval.type) return func.call({ lval, rval }, activeModule);
    }
    return nullopt;
}

optional<Value> notEqual(Value& lval, Value& rval) {
    Value l = lval.actualValue();
    Value r = rval.actualValue();
    if (l.type.isNumber() && r.type.isNumber()) {
        castNumberForBiop(l, r);
        LLVMValueRef val;
        if (l.type.isFloat()) {
            val = LLVMBuildFCmp(builder, LLVMRealONE, l.llvmValue, r.llvmValue, "neq");
        } else {
            val = LLVMBuildICmp(builder, LLVMIntNE, l.llvmValue, r.llvmValue, "neq");
        }
        return Value(val, nameToType["bool"].front(), l.module, l.constant && r.constant);
    }
    vector<Function>& funcs = nameToFunction["notEqual"];
    for (int i = 0; i < funcs.size(); i++) {
        Function& func = funcs[i];
        if (func.paramNames.size() != 2) continue;
        if (func.paramTypes[0] == lval.type && func.paramTypes[1] == rval.type) return func.call({ lval, rval }, activeModule);
    }
    return nullopt;
}

Value as(Value& lval, Type& rval) {
    Value l = lval.actualValue();
    int rbitsize = LLVMSizeOfTypeInBits(LLVMGetModuleDataLayout(activeModule->llvmModule), rval.llvmType);
    int lbitsize = LLVMSizeOfTypeInBits(LLVMGetModuleDataLayout(activeModule->llvmModule), lval.type.llvmType);
    if (lbitsize < rbitsize) {
        l.llvmValue = LLVMBuildZExt(builder, l.llvmValue, rval.llvmType, "upcast");
    } else {
        l.llvmValue = LLVMBuildTrunc(builder, l.llvmValue, rval.llvmType, "downcast");
    }
    l.llvmValue = LLVMBuildBitCast(builder, l.llvmValue, rval.llvmType, "bitcast");
    l.type = rval;
    return l;
}

optional<Value> to(Value& lval, Type& rval) {
    Value l = lval.actualValue();
    return l.cast(rval);
}

optional<Value> mul(Value& lval, Value& rval) {
    Value l = lval.actualValue();
    Value r = rval.actualValue();
    if (l.type.isNumber() && r.type.isNumber()) {
        castNumberForBiop(l, r);
        LLVMValueRef val;
        if (l.type.isFloat()) {
            val = LLVMBuildFMul(builder, l.llvmValue, r.llvmValue, "mulf");
        } else {
            val = LLVMBuildMul(builder, l.llvmValue, r.llvmValue, "mulf");
        }
        return Value(val, l.type, l.module, l.constant && r.constant);
    }
    vector<Function>& funcs = nameToFunction["mul"];
    for (int i = 0; i < funcs.size(); i++) {
        Function& func = funcs[i];
        if (func.paramNames.size() != 2) continue;
        if (func.paramTypes[0] == lval.type && func.paramTypes[1] == rval.type) return func.call({ lval, rval }, activeModule);
    }
    return nullopt;
}

optional<Value> div(Value& lval, Value& rval) {
    Value l = lval.actualValue();
    Value r = rval.actualValue();
    if (l.type.isNumber() && r.type.isNumber()) {
        castNumberForBiop(l, r);
        LLVMValueRef val;
        if (l.type.isFloat()) {
            val = LLVMBuildFDiv(builder, l.llvmValue, r.llvmValue, "divf");
        } else {
            if (l.type.isUInt()) val = LLVMBuildUDiv(builder, l.llvmValue, r.llvmValue, "divf");
            else
                val = LLVMBuildSDiv(builder, l.llvmValue, r.llvmValue, "divf");
        }
        return Value(val, l.type, l.module, l.constant && r.constant);
    }
    vector<Function>& funcs = nameToFunction["div"];
    for (int i = 0; i < funcs.size(); i++) {
        Function& func = funcs[i];
        if (func.paramNames.size() != 2) continue;
        if (func.paramTypes[0] == lval.type && func.paramTypes[1] == rval.type) return func.call({ lval, rval }, activeModule);
    }
    return nullopt;
}

optional<Value> and (Value & lval, Value& rval) {
    optional<Value> l = lval.toBool();
    if (!l.has_value()) return nullopt;
    optional<Value> r = rval.toBool();
    if (!r.has_value()) return nullopt;
    l.value().llvmValue = LLVMBuildAnd(builder, l.value().llvmValue, r.value().llvmValue, "cmp");
    return l;
}

optional<Value> or (Value & lval, Value& rval) {
    optional<Value> l = lval.toBool();
    if (!l.has_value()) return nullopt;
    optional<Value> r = rval.toBool();
    if (!r.has_value()) return nullopt;
    l.value().llvmValue = LLVMBuildOr(builder, l.value().llvmValue, r.value().llvmValue, "cmp");
    return l;
}
