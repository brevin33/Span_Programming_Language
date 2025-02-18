#pragma once
#include "utils.h"

class Module;

class Type {
public:
    Type();

    Type(LLVMTypeRef llvmType, const string& name, Module* module);

    Type(const string& name, Module* module);

    Type::~Type();

    bool isInt();

    bool isUInt();

    bool isFloat();

    bool isNumber();

    bool isPtr();

    bool isRef();

    Type actualType();

    Type dereference();

    Type ptr();

    Type ref();

    u64 getNumberWidth();

    bool operator==(const Type& other) const {
        return (name == other.name) && (module == other.module);
    }

    bool operator!=(const Type& other) const {
        return (name != other.name) || (module != other.module);
    }

public:
    LLVMTypeRef llvmType;
    string name;
    Module* module;

private:
private:
};