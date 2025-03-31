#pragma once
#include "utils.h"
#include <unordered_map>
#include <string>

enum TypeKind { tk_bool, tk_int, tk_array, tk_float, tk_uint, tk_void, tk_ref, tk_ptr, tk_struct, tk_enum, tk_struct_impl, tk_enum_impl, tk_err };

struct Type {
    Type() {
        id = -1;
    }
    Type(int id)
        : id(id) {
    }
    int id;
    TypeKind getTypeKind();
    Type getRef();
    Type getPtr();
    Type getDeRef();
    Type getArray(int size);
    Type getActualType();
    LLVMTypeRef getLLVMType();
    vector<Type>& getStructElements();
    vector<string>& getStructElementNames();
    vector<Type>& getEnumElements();
    vector<string>& getEnumElementNames();
    int getNumberWidth();
    int getTypeSize();

    bool isIntLike();
    bool isNumLike();
    bool isStructLike();
    bool isEnumLike();
    string getName();
};

struct Types {
    unordered_map<string, Type> nameToType;
    vector<string> typeidToName;
    vector<LLVMTypeRef> typeidTollvm;
    vector<TypeKind> typeidToKind;
    vector<vector<Type>> typeidToEles;
    vector<vector<string>> typeidToEleNames;
    vector <
};


optional<Type> getTypeFromName(string typeName);

Type createType(string name, LLVMTypeRef llvmType, TypeKind kind, const vector<Type>& eles = {}, const vector<string>& eleNames = {});

optional<Type> parseType(bool logErrors = true, bool stopAtComma = false, bool stopAtOr = false);
