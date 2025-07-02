#include "span_parser/type.h"
#include "span_parser.h"

Type* getNumberType(u64 bits, TypeKind kind) {
    char buffer[4096];
    char* bitsNumber = uintToString(bits, buffer);
    u64 bitNumberLength = strlen(bitsNumber);
    if (context.baseTypesCount >= context.baseTypesCapacity) {
        context.baseTypes = reallocArena(context.arena, sizeof(Type) * context.baseTypesCapacity, context.baseTypes, sizeof(Type) * context.baseTypesCount * 2);
        context.baseTypesCapacity *= 2;
    }
    Type* type = context.baseTypes + context.baseTypesCount++;
    type->kind = kind;
    // it is fine to use inttype because bool uint float are all the same as int
    type->intType.bits = bits;
    type->_name = allocArena(context.arena, bitNumberLength + 2);
    switch (kind) {
        case tk_int: {
            type->_name[0] = 'i';
            break;
        }
        case tk_uint: {
            type->_name[0] = 'u';
            break;
        }
        case tk_float: {
            type->_name[0] = 'f';
            break;
        }
        case tk_bool: {
            type->_name[0] = 'b';
            break;
        }
        default: {
            massert(false, "invalid type");
            break;
        }
    };
    memcpy(type->_name + 1, bitsNumber, bitNumberLength + 1);
    return type;
}

Type* TypeCreateGloablAlias(Type* baseType, char* name) {
    if (context.baseTypesCount >= context.baseTypesCapacity) {
        context.baseTypes = reallocArena(context.arena, sizeof(Type) * context.baseTypesCapacity, context.baseTypes, sizeof(Type) * context.baseTypesCount * 2);
        context.baseTypesCapacity *= 2;
    }
    Type* type = context.baseTypes + context.baseTypesCount++;
    type->kind = tk_alias;
    type->aliasType.type = baseType;
    u64 nameLength = strlen(name);
    type->_name = allocArena(context.arena, nameLength + 2);
    memcpy(type->_name, name, nameLength + 1);
    return type;
}

char* TypeGetName(Type* type, char* buffer) {
    switch (type->kind) {
            // all the types with names are here
        case tk_struct:
        case tk_enum:
        case tk_union:
        case tk_alias:
        case tk_distinct:
        case tk_uint:
        case tk_float:
        case tk_bool:
        case tk_interface:
        case tk_int:
            memcpy(buffer, type->_name, strlen(type->_name) + 1);
            return buffer;
        // all the types that must derive the name from the base types bellow
        case tk_ref:
        case tk_array:
        case tk_map:
        case tk_list:
        case tk_ptr: {
            return NULL;
        }
        case tk_invalid: {
            massert(false, "should never be trying to get name of invalid type");
            return NULL;
        }
    }
}
