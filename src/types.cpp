#include "types.h"
#include "span.h"

TypeKind Type::getTypeKind() {
    return c.types.typeidToKind[id];
}

Type Type::getRef() {
    assert(getTypeKind() != tk_void);
    string newName = getName() + "&";
    auto it = c.types.nameToType.find(newName);
    if (it != c.types.nameToType.end()) {
        return Type(it->second);
    }
    LLVMTypeRef llvm = LLVMPointerType(LLVMInt1Type(), 0);
    return createType(newName, llvm, tk_ref, getStructElements(), getStructElementNames());
}

Type Type::getPtr() {
    string newName = getName() + "*";
    auto it = c.types.nameToType.find(newName);
    if (it != c.types.nameToType.end()) {
        return Type(it->second);
    }
    LLVMTypeRef llvm = LLVMPointerType(LLVMInt1Type(), 0);
    return createType(newName, llvm, tk_ptr, getStructElements(), getStructElementNames());
}

Type Type::getDeRef() {
    assert(getTypeKind() == tk_ref || getTypeKind() == tk_ptr || getTypeKind() == tk_array);
    string newName = getName();
    if (getTypeKind() == tk_array) {
        while (newName.back() != '[') {
            newName.pop_back();
        }
        newName.pop_back();
    } else {
        newName.pop_back();
    }
    auto it = c.types.nameToType.find(newName);
    assert(it != c.types.nameToType.end());
    return it->second;
}

Type Type::getArray(int size) {
    assert(getTypeKind() != tk_void);
    string newName = getName() + "[" + to_string(size) + "]";
    auto it = c.types.nameToType.find(newName);
    if (it != c.types.nameToType.end()) {
        return it->second;
    }
    LLVMTypeRef llvm = LLVMVectorType(getLLVMType(), size);
    return createType(newName, llvm, tk_array, getStructElements(), getStructElementNames());
}

Type Type::getActualType() {
    Type type = *this;
    while (type.getTypeKind() == tk_ref) {
        type = type.getDeRef();
    }
    return type;
}

LLVMTypeRef Type::getLLVMType() {
    return c.types.typeidTollvm[id];
}

vector<string>& Type::getStructElementNames() {
    return c.types.typeidToEleNames[id];
}

vector<string>& Type::getEnumElementNames() {
    return c.types.typeidToEleNames[id];
}

int Type::getNumberWidth() {
    assert(isNumLike());
    string n = getName();
    n.front() = '0';
    return stoull(n);
}

int Type::getTypeSize() {
    if (getTypeKind() == tk_void) return 0;
    return LLVMSizeOfTypeInBits(LLVMGetModuleDataLayout(c.activeModule->llvmModule), getLLVMType());
}

vector<Type>& Type::getStructElements() {
    return c.types.typeidToEles[id];
}

vector<Type>& Type::getEnumElements() {
    return c.types.typeidToEles[id];
}

bool Type::isIntLike() {
    return getTypeKind() == tk_int || getTypeKind() == tk_uint;
}

bool Type::isNumLike() {
    return getTypeKind() == tk_int || getTypeKind() == tk_uint || getTypeKind() == tk_float;
}

bool Type::isStructLike() {
    return getTypeKind() == tk_struct_impl || getTypeKind() == tk_struct;
}

bool Type::isEnumLike() {
    return getTypeKind() == tk_enum_impl || getTypeKind() == tk_enum;
}

string Type::getName() {
    return c.types.typeidToName[id];
}

optional<Type> getTypeFromName(string typeName) {
    auto it = c.types.nameToType.find(typeName);
    if (it != c.types.nameToType.end()) {
        return it->second;
    }
    // Check if it is a number type
    if (typeName.front() == 'f' || typeName.front() == 'i' || typeName.front() == 'u') {
        int numSize = 0;
        int i = 1;
        while (i < typeName.size()) {
            if (!isdigit(typeName[i])) {
                break;
            }
            numSize = numSize * 10 + (typeName[i] - '0');
            i++;
        }
        if (i == typeName.size()) {
            if (typeName.front() == 'f') {
                if (numSize == 16) {
                    return createType("f16", LLVMHalfType(), tk_float);
                } else if (numSize == 32) {
                    return createType("f32", LLVMFloatType(), tk_float);
                } else if (numSize == 64) {
                    return createType("f64", LLVMDoubleType(), tk_float);
                } else {
                    return nullopt;
                }
            }
            return createType(typeName, LLVMIntType(numSize), typeName.front() == 'i' ? tk_int : tk_uint);
        }
    }
    if (typeName == "void") {
        return createType("void", LLVMVoidType(), tk_void);
    }
    if (typeName == "bool") {
        return createType("bool", LLVMInt1Type(), tk_bool);
    }
    if (typeName == "error") {
        return createType("error", LLVMInt1Type(), tk_err);
    }
    return nullopt;
}

Type createType(string name, LLVMTypeRef llvmType, TypeKind kind, const vector<Type>& eles, const vector<string>& eleNames) {
    int id = c.types.typeidToName.size();
    c.types.typeidToName.push_back(name);
    c.types.typeidTollvm.push_back(llvmType);
    c.types.typeidToKind.push_back(kind);
    c.types.typeidToEleNames.push_back(eleNames);
    c.types.typeidToEles.push_back(eles);
    c.types.nameToType[name] = Type(id);
    return Type(id);
}

optional<Type> typeFromTokens(bool logErrors, bool stopAtComma, bool stopAtOr) {
    int startPos = c.pos;
    if (c.tokens[c.pos].type != tt_id) {
        if (logErrors) logError("Expected type", c.tokens[c.pos]);
        c.pos = startPos;
        return nullopt;
    }
    string baseName = c.tokens[c.pos].getStr();
    optional<Type> base = getTypeFromName(baseName);
    if (!base.has_value()) {
        if (logErrors) logError("No type with this name", c.tokens[c.pos]);
        c.pos = startPos;
        return nullopt;
    }
    Type type = base.value();
    c.pos++;
    while (true) {
        switch (c.tokens[c.pos].type) {
            case tt_mul: {
                type = type.getPtr();
                c.pos++;
                break;
            }
            case tt_bitand: {
                type = type.getRef();
                c.pos++;
                break;
            }
            case tt_lbar: {
                c.pos++;
                if (c.tokens[c.pos].type != tt_int) {
                    if (logErrors) logError("Expected number", c.tokens[c.pos]);
                    c.pos = startPos;
                    return nullopt;
                }
                u64 number = c.tokens[c.pos].data.uint;
                c.pos++;
                if (c.tokens[c.pos].type != tt_rbar) {
                    if (logErrors) logError("Expected ]", c.tokens[c.pos]);
                    c.pos = startPos;
                    return nullopt;
                }
                c.pos++;
                type = type.getArray(number);
                break;
            }
            case tt_bitor: {
                if (stopAtOr) return type;
                c.pos++;
                vector<Type> impleStructTypes;
                impleStructTypes.push_back(type);
                while (true) {
                    optional<Type> nextType = typeFromTokens(logErrors, false, true);
                    if (!nextType.has_value()) {
                        c.pos = startPos;
                        return nullopt;
                    }
                    impleStructTypes.push_back(nextType.value());
                    if (c.tokens[c.pos].type != tt_bitor) break;
                    c.pos++;
                }
                string typeName = "(";
                vector<string> structElmName;
                vector<int> enumValues;
                for (int i = 0; i < impleStructTypes.size(); i++) {
                    typeName += impleStructTypes[i].getName() + "|";
                    structElmName.push_back(impleStructTypes[i].getName());
                    enumValues.push_back(i);
                }
                typeName += ")";
                LLVMTypeRef elTypes[50];
                for (int i = 0; i < structElmName.size(); i++) {
                    elTypes[i] = impleStructTypes[i].getLLVMType();
                }
                LLVMTypeRef llvmType = LLVMStructType(elTypes, structElmName.size(), 0);
                optional<Type> t = getTypeFromName(typeName);
                if (t.has_value()) {
                    type = t.value();
                } else {
                    type = createType(typeName, llvmType, tk_enum_impl, impleStructTypes, structElmName);
                }
                return type;
            }
            case tt_com: {
                if (stopAtComma) return type;
                c.pos++;
                vector<Type> impleStructTypes;
                impleStructTypes.push_back(type);
                while (true) {
                    optional<Type> nextType = typeFromTokens(logErrors, true, stopAtOr);
                    if (!nextType.has_value()) {
                        c.pos = startPos;
                        return nullopt;
                    }
                    impleStructTypes.push_back(nextType.value());
                    if (c.tokens[c.pos].type != tt_com) break;
                    if (c.tokens[c.pos].type != tt_bitor) break;
                    c.pos++;
                }
                string typeName = "(";
                vector<string> structElmName;
                for (int i = 0; i < impleStructTypes.size(); i++) {
                    typeName += impleStructTypes[i].getName();
                    structElmName.push_back(to_string(i));
                }
                typeName += ")";
                LLVMTypeRef elTypes[50];
                for (int i = 0; i < structElmName.size(); i++) {
                    elTypes[i] = impleStructTypes[i].getLLVMType();
                }
                LLVMTypeRef llvmType = LLVMStructType(elTypes, structElmName.size(), 0);
                optional<Type> t = getTypeFromName(typeName);
                if (t.has_value()) {
                    type = t.value();
                } else {
                    type = createType(typeName, llvmType, tk_struct_impl, impleStructTypes, structElmName);
                }
                return type;
            }
            default: {
                return type;
            }
        }
    }
}
