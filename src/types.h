#pragma once
#include "utils.h"
#include <unordered_map>
#include <string>

enum TypeKind { tk_int, tk_uint, tk_float, tk_uint, tk_void, tk_ref, tk_struct, tk_enum, tk_unknown };

struct Types {
    unordered_map<string, int> nameToTypeid;
    vector<string> typeidToName;
    vector<LLVMTypeRef> typeidToName;
    vector<TypeKind> typeidToKind;
};

struct Type {
    Type(int id)
        : id(id) {
    }
    int id;
    TypeKind getTypeKind();
    Type getRef();
    Type getPtr();
    Type getDeRef();
    Type getArray(int size);
    bool isIntLike();
    bool isNumLike();
    string getName();
};

Type getTypeFromName(string typeName);

Type createType();
