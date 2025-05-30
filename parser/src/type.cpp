#include "parser/type.h"
#include "parser.h"
#include <cassert>
#include <unordered_map>

std::vector<Type> gTypes;

std::unordered_map<std::string, TypeId> gTypeNameToId;

TypeKind TypeId::getKind() {
    assert(id < gTypes.size() && "TypeId is out of bounds");
    return gTypes[id].kind;
}
bool TypeId::isInt() {
    assert(id < gTypes.size() && "TypeId is out of bounds");
    return gTypes[id].isInt();
}
bool TypeId::isNumber() {
    assert(id < gTypes.size() && "TypeId is out of bounds");
    return gTypes[id].isNumber();
}
u64 TypeId::getNumberSize() {
    assert(id < gTypes.size() && "TypeId is out of bounds");
    assert(gTypes[id].isNumber() && "TypeId is not a number type");
    return gTypes[id].number.numberSize;
}
std::vector<Type>& TypeId::getStructMembers() {
    assert(id < gTypes.size() && "TypeId is out of bounds");
    assert(gTypes[id].kind == tk_struct && "TypeId is not a struct type");
    return gTypes[id].structType.members;
}
std::vector<EnumData>& TypeId::getEnumData() {
    assert(id < gTypes.size() && "TypeId is out of bounds");
    assert(gTypes[id].kind == tk_struct && "TypeId is not a struct type");
    return gTypes[id].enumType.underlyingTypes;
}
TypeId TypeId::getPointedToType() {
    assert(id < gTypes.size() && "TypeId is out of bounds");
    assert(gTypes[id].kind == tk_pointer && "TypeId is not a pointer type");
    return gTypes[id].pointer.pointedType;
}

TypeId getTypeFromName(std::string& name, bool& error) {
    auto it = gTypeNameToId.find(name);
    if (it != gTypeNameToId.end()) {
        return it->second;
    }
    error = true;
    return TypeId { 0 };  // Invalid TypeId
}

void addType(Type& type) {
    TypeId typeId = { gTypes.size() };
    gTypes.push_back(type);
    gTypeNameToId[type.name] = typeId;

    // Ensure the type is registered correctly
    assert(gTypeNameToId[type.name].id == typeId.id && "Type registration failed");
}
