#include "types.h"
#include "span.h"

TypeKind Type::getTypeKind() {
    return context.types.typeidToKind[id];
}

bool Type::isIntLike() {
    return getTypeKind() == tk_int || getTypeKind() == tk_uint;
}

bool Type::isNumLike() {
    return getTypeKind() == tk_int || getTypeKind() == tk_uint || getTypeKind() == tk_float;
}

string Type::getName() {
    return context.types.typeidToName[id];
}

Type getTypeFromName(string typeName) {
    return Type(context.types.nameToTypeid[typeName]);
}
