#include "type.h"
#include "span.h"
Type::Type() {
}

Type::Type(LLVMTypeRef llvmType, const string& name, Module* module) {
    this->llvmType = llvmType;
    this->name = name;
    this->module = module;
    if (nameToType[name].size() == 0) nameToType[name].push_back(*this);
}

Type::Type(const string& name, vector<Type>& structTypes, vector<string>& struceElmNames, Module* module, const vector<Type>& tempalteTypes) {
    this->name = name;
    this->module = module;
    this->elemTypes = structTypes;
    this->elemNames = struceElmNames;
    vector<LLVMTypeRef> llvmTypes;
    for (int i = 0; i < structTypes.size(); i++) {
        llvmTypes.push_back(structTypes[i].llvmType);
    }
    this->llvmType = LLVMStructCreateNamed(LLVMGetGlobalContext(), name.c_str());
    this->templateTypes = tempalteTypes;
    LLVMStructSetBody(llvmType, llvmTypes.data(), llvmTypes.size(), false);
    if (nameToType[name].size() == 0) nameToType[name].push_back(*this);
}

Type::Type(const string& name, vector<Type>& enumTypes, vector<string>& enumElmNames, vector<int> enumElmValues, Module* module, bool staticEnum, const vector<Type>& tempalteTypes) {
    this->templateTypes = tempalteTypes;
    this->name = name;
    this->module = module;
    this->elemTypes = enumTypes;
    this->elemNames = enumElmNames;
    this->enumValues = enumElmValues;
    vector<LLVMTypeRef> llvmTypes;
    this->staticEnum = staticEnum;
    llvmTypes.push_back(LLVMInt32Type());
    LLVMTypeRef largestType;
    int largestTypeSize = 0;
    for (int i = 0; i < enumTypes.size(); i++) {
        if (enumTypes[i].getBitWidth() > largestTypeSize) {
            largestTypeSize = enumTypes[i].getBitWidth();
            largestType = enumTypes[i].llvmType;
        }
    }
    llvmTypes.push_back(largestType);
    this->llvmType = LLVMStructCreateNamed(LLVMGetGlobalContext(), name.c_str());
    LLVMStructSetBody(llvmType, llvmTypes.data(), llvmTypes.size(), false);
    if (nameToType[name].size() == 0) nameToType[name].push_back(*this);
}


Type::Type(const string& name, Module* module) {
    // this is trash but i guess llvm doesn't keep pointer type info any more ????
    int baseTypeEnd = name.size();
    for (int i = 0; i < name.size(); i++) {
        if (name[i] == '*' || name[i] == '&' || name[i] == '[' || name[i] == '^') {
            baseTypeEnd = i;
            break;
        }
        if (name[i] == '(') {
            while (name[i] != ')')
                i++;
        }
        if (name[i] == '<') {
            while (name[i] != '>')
                i++;
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
            case '[': {
                i++;
                int num = 0;
                while (name[i] != ']') {
                    num *= 10;
                    num += name[i] - '0';
                    i++;
                }
                baseType = baseType.vec(num);
                break;
            }
            default: {
                assert(false);
            }
        }
    }
    *this = baseType;
}

Type::~Type() {
}

bool Type::isInt() {
    if (name[0] != 'i') return false;
    for (int i = 1; i < name.size(); i++) {
        if (name[i] < '0' || name[i] > '9') return false;
    }
    return true;
}

bool Type::isUInt() {
    if (name[0] != 'u') return false;
    for (int i = 1; i < name.size(); i++) {
        if (name[i] < '0' || name[i] > '9') return false;
    }
    return true;
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

bool Type::isEnum() {
    return enumValues.size() != 0;
}

bool Type::isStruct() {
    return elemNames.size() != 0 && enumValues.size() == 0;
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
    LLVMTypeRef ref = LLVMPointerType(LLVMInt32Type(), 0);
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

Type Type::vecOfType() {
    assert(isVec());
    Type t = *this;
    while (t.name.back() != '[') {
        t.name.pop_back();
    }
    t.name.pop_back();
    t.llvmType = LLVMGetElementType(llvmType);
    return t;
}

u64 Type::getNumberWidth() {
    assert(isInt() || isUInt());
    string n = name;
    n.erase(0, 1);
    return stoull(n);
}

u64 Type::getBitWidth() {
    if (name == "void") return 0;
    return LLVMSizeOfTypeInBits(LLVMGetModuleDataLayout(module->llvmModule), llvmType);
}
