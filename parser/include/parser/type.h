#pragma once
#include "nice_ints.h"
#include <vector>
#include <string>

enum TypeKind {
    tk_void,
    tk_sint,
    tk_uint,
    tk_float,
    tk_struct,
    tk_pointer,
    tk_array,
};

struct EnumData;
struct Type;
extern std::vector<Type> gTypes;


struct TypeId {
    u64 id;
    bool operator==(const TypeId& other) const {
        return id == other.id;
    }
    bool operator!=(const TypeId& other) const {
        return id != other.id;
    }
    TypeKind getKind();
    bool isInt();
    bool isNumber();
    u64 getNumberSize();
    std::vector<Type>& getStructMembers();
    std::vector<EnumData>& getEnumData();
    TypeId getPointedToType();
};


struct Type {

    bool isInt() const {
        return kind == tk_sint || kind == tk_uint;
    }

    bool isNumber() const {
        return kind == tk_sint || kind == tk_uint || kind == tk_float;
    }

    TypeKind kind;
    std::string name;
    union {
        struct {
            u64 numberSize;
        } number;
        struct {
            std::vector<Type> members;
        } structType;
        struct {
            std::vector<EnumData> underlyingTypes;
        } enumType;
        struct {
            const TypeId pointedType;
        } pointer;
    };
};

struct EnumData {
    TypeId type;
    i64 idValue;
};

TypeId getTypeFromName(std::string& name, bool& error);

void addType(Type& type);
