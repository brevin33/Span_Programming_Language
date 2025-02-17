#pragma once
#include "utils.h"

class Module;

class Type {
public:
    Type();

    Type(LLVMTypeRef llvmType, const string& name, Module* module);

    Type(Type type, const string& name, Module* module);

    Type::~Type();

    bool isInt();

    bool isUInt();

    bool isFloat();

    bool isNumber();

    bool isPtr();

    bool isRef();

    Type dereference();

    Type ptr();

    Type ref();

public:
    LLVMTypeRef llvmType;
    string name;
    Type* alias = nullptr;
    Module* module;

private:
private:
};