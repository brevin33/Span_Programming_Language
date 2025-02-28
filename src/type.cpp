#include "type.h"
#include "span.h"
Type::Type() {
}

Type::Type(LLVMTypeRef llvmType, const string& name, Module* module) {
    this->llvmType = llvmType;
    this->name = name;
    this->module = module;
}

Type::Type(const string& name, vector<Type>& structTypes, vector<string>& struceElmNames, Module* module) {
    this->name = name;
    this->module = module;
    this->structTypes = structTypes;
    this->structElemNames = struceElmNames;
    vector<LLVMTypeRef> llvmTypes;
    for (int i = 0; i < structTypes.size(); i++) {
        llvmTypes.push_back(structTypes[i].llvmType);
    }
    this->llvmType = LLVMStructCreateNamed(LLVMGetGlobalContext(), name.c_str());
    LLVMStructSetBody(llvmType, llvmTypes.data(), llvmTypes.size(), false);
}


Type::Type(const string& name, Module* module) {
    // this is trash but i guess llvm doesn't keep pointer type info any more ????
    int baseTypeEnd = name.size();
    for (int i = 0; i < name.size(); i++) {
        if (name[i] == '*' || name[i] == '&' || name[i] == '[' || name[i] == '^') {
            baseTypeEnd = i;
            break;
        }
    }
    string baseTypeName = name;
    baseTypeName.resize(baseTypeEnd);
    auto baseTypePoss = nameToType[baseTypeName];
    Type baseType;
    bool foundSomthing = false;
    for (int i = 0; i < baseTypePoss.size(); i++) {
        if (baseTypePoss[i].module == module) {
            baseType = baseTypePoss[i];
            foundSomthing = true;
        }
    }
    assert(foundSomthing);
    for (int i = baseTypeEnd; i < name.size(); i++) {
        switch (name[i]) {
            case '*': {
                baseType = baseType.ptr();
                break;
            }
            case '&': {
                baseType = baseType.ref();
                break;
            }
            default: {
                assert(false);
            }
        }
    }
    this->llvmType = baseType.llvmType;
    this->name = baseType.name;
    this->module = baseType.module;
}

Type::~Type() {
}

bool Type::isInt() {
    if (name[0] != 'i') return false;
    for (int i = 1; i < name.size(); i++) {
        if (name[i] < '0' || name[i] > '9') return false;
    }
    return true;
    return false;
}

bool Type::isUInt() {
    if (name[0] != 'u') return false;
    for (int i = 1; i < name.size(); i++) {
        if (name[i] < '0' || name[i] > '9') return false;
    }
    return true;
    return false;
}

bool Type::isFloat() {
    if (name[0] != 'f') return false;
    for (int i = 1; i < name.size(); i++) {
        if (name[i] < '0' || name[i] > '9') return false;
    }
    return true;
}

bool Type::isNumber() {
    if (name[0] != 'f' && name[0] != 'i' && name[0] != 'u') return false;
    for (int i = 1; i < name.size(); i++) {
        if (name[i] < '0' || name[i] > '9') return false;
    }
    return true;
}

bool Type::isPtr() {
    if (name.back() == '*') return true;
    return false;
}

bool Type::isRef() {
    if (name.back() == '&') return true;
    return false;
}

bool Type::isVec() {
    if (name.back() == ']') return true;
    return false;
}

Type Type::actualType() {
    Type t = *this;
    while (t.isRef()) {
        t = t.dereference();
    }
    return t;
}

Type Type::dereference() {
    assert(name.back() == '&' || name.back() == '*');
    if (name.back() == '&') {
        string newName = name;
        newName.resize(newName.size() - 1);
        return Type(newName, module);
    } else {
        Type t = *this;
        t.name.back() = '&';
        return t;
    }
}

Type Type::ptr() {
    LLVMTypeRef ref = LLVMPointerType(llvmType, 0);
    string newName = name + '*';
    Type t = *this;
    t.name = newName;
    t.llvmType = ref;
    return t;
}

Type Type::ref() {
    LLVMTypeRef ref = LLVMPointerType(llvmType, 0);
    string newName = name + '&';
    Type t = *this;
    t.name = newName;
    t.llvmType = ref;
    return t;
}

Type Type::vec(u64 num) {
    LLVMTypeRef ref = LLVMVectorType(llvmType, num);
    string newName = name + '[' + to_string(num) + ']';
    return Type(ref, newName, module);
}

u64 Type::getNumberWidth() {
    assert(isInt() || isUInt());
    string n = name;
    n.erase(0, 1);
    return stoull(n);
}
