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
    return nullopt;
}
