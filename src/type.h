#pragma once
#include "utils.h"

class Module;

class Type {
public:
    Type();

    Type(LLVMTypeRef llvmType, const string& name, Module* module);

    Type(const string& name, vector<Type>& structTypes, vector<string>& struceElmNames, Module* module);

    Type(const string& name, vector<Type>& enumTypes, vector<string>& enumElmNames, vector<int> enumValues, Module* module);

    Type(const string& name, Module* module);

    Type::~Type();

    bool isInt();

    bool isUInt();

    bool isFloat();

    bool isNumber();

    bool isPtr();

    bool isRef();

    bool isVec();

    bool isEnum();

    bool isStruct();

    Type actualType();

    Type dereference();

    Type ptr();

    Type ref();

    Type vec(u64 num);

    u64 getNumberWidth();

    u64 getBitWidth();

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
    vector<Type> elemTypes;
    vector<string> elemNames;
    vector<int> enumValues;

private:
private:
};