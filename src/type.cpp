#include "type.h"
Type::Type() {
}

Type::Type(LLVMTypeRef llvmType, const string& name, Module* module) {
    this->llvmType = llvmType;
    this->name = name;
    this->module = module;
}

Type::Type(Type type, const string& name, Module* module) {
    this->llvmType = type.llvmType;
    this->name = name;
    this->module = module;
    this->alias = new Type;
    *this->alias = type;
}

Type::~Type() {
    if (alias != nullptr) delete alias;
}

bool Type::isInt() {
    //TODO
    return false;
}

bool Type::isUInt() {
    //TODO
    return false;
}

bool Type::isFloat() {
    //TODO
    return false;
}

bool Type::isNumber() {
    //TODO
    return false;
}

bool Type::isPtr() {
    //TODO
    return false;
}

bool Type::isRef() {
    if (name.back() == '&') return true;
    return false;
}

Type Type::dereference() {
    //TODO
    return {};
}

Type Type::ptr() {
    //TODO
    return {};
}

Type Type::ref() {
    LLVMTypeRef ref = LLVMPointerType(llvmType, 0);
    string newName = name + '&';
    return Type(ref, newName, module);
}
