#include "span_parser/type.h"
#include "span_parser.h"

Type* TypeFromTokens(SpanFile* file, Token** tokens, bool logError) {
    Token* token = *tokens;
    char buffer[4096];
    char* namespace = getNameSpaceFromTokens(tokens, buffer);
    if (token->type != tt_id) {
        if (logError) logErrorTokens(token, 1, "expected type name");
        return NULL;
    }
    char buffer2[4096];
    char* name = tokenGetString(*token, buffer2);
    token++;
    TemplateDefinition* templateDefinition = createTemplateDefinitionFromTokens(file->arena, &token);

    Type* type = TypeFromNameNamespaceTemplate(context.activeProject, name, namespace, templateDefinition);

    while (true) {
        //TODO: pointers, arrays, functions, etc
    }

    *tokens = token;
}

Type* TypeCreateFromTokens(SpanFile* file, Token** tokens) {
    massert(false, "not implemented");
    Token* token = *tokens;
    switch (token->type) {
        case tt_struct: {
        }
        default: {
            return NULL;
        }
    }


    *tokens = token;
    return NULL;
}

static Type* TypeInProject(SpanProject* project, char* name, char* namespace) {
    if (namespace != NULL) {
        if (strcmp(namespace, project->name) != 0) {
            return NULL;
        }
    }
    for (u64 i = 0; i < project->fileCount; i++) {
        for (u64 j = 0; j < project->files[i].typesCount; j++) {
            Type* type = &project->files[i].types[j];
            if (strcmp(type->name, name) == 0) {
                return type;
            }
        }
    }
    return NULL;
}

Type* TypeFromNameNamespaceTemplate(SpanProject* project, char* name, char* namespace, TemplateDefinition* templateDefinition) {
    Type* type = TypeInProject(project, name, namespace);
    if (type == NULL) {
        for (u64 i = 0; i < project->childCount; i++) {
            SpanProject* child = &project->children[i];
            type = TypeFromNameNamespaceTemplate(child, name, namespace, templateDefinition);
            if (type != NULL) break;
        }
    }
    if (type == NULL) return NULL;
    //TODO: handel template if it exists
    return type;
}

Type* getNumberType(u64 bits, TypeKind kind) {
    massert(kind == tk_int || kind == tk_uint || kind == tk_float || kind == tk_bool, "can't create number type with this kind");
    massert(kind != tk_float || (bits == 16 || bits == 32 || bits == 64), "floats can only be 16, 32 or 64 bits");
    massert(kind != tk_bool || bits == 1, "bools can only be 1 bit");
    for (u64 i = 0; i < context.baseTypesCount; i++) {
        Type* type = context.baseTypes + i;
        if (type->kind == tk_int && type->number.bits == bits) {
            return type;
        }
    }

    if (context.baseTypesCount >= context.baseTypesCapacity) {
        context.baseTypes = reallocArena(context.arena, sizeof(Type) * context.baseTypesCapacity, context.baseTypes, sizeof(Type) * context.baseTypesCount * 2);
        context.baseTypesCapacity *= 2;
    }
    Type* type = context.baseTypes + context.baseTypesCount++;

    type->arena = context.arena;

    type->kind = kind;

    type->templateDefinition = NULL;

    u64 digits = countDigits(bits);

    if (kind != tk_bool) {
        type->name = allocArena(context.arena, digits + 2);
        if (kind == tk_int) {
            type->name[0] = 'i';
        } else if (kind == tk_uint) {
            type->name[0] = 'u';
        } else if (kind == tk_float) {
            type->name[0] = 'f';
        } else if (kind == tk_bool) {
            type->name[0] = 'b';
        }
        uintToString(bits, type->name + 1);
        type->name[digits + 1] = '\0';
    }

    type->number.bits = bits;

    type->alises = NULL;

    type->alisesCount = 0;

    type->origin = to_original;

    return type;
}

Type* TypeCreateAliasGlobal(Type* baseType, const char* name) {
    Type* type = context.baseTypes + context.baseTypesCount++;
    *type = *baseType;
    u64 nameLength = strlen(name);
    type->arena = createArena(context.arena, (nameLength + 4) * 2);
    type->name = allocArena(type->arena, nameLength + 1);
    memcpy(type->name, name, nameLength + 1);
    type->baseType = baseType;
    type->origin = to_alias;
    type->alises = NULL;
    type->alisesCount = 0;
    type->templateDefinition = NULL;

    if (baseType->alises == NULL) {
        baseType->alises = allocArena(baseType->arena, sizeof(Type*) * 1);
        baseType->alisesCount = 1;
    } else {
        baseType->alises = reallocArena(baseType->arena, sizeof(Type*) * baseType->alisesCount, baseType->alises, sizeof(Type*) * (baseType->alisesCount + 1));
        baseType->alisesCount++;
    }
    baseType->alises[baseType->alisesCount - 1] = type;

    return type;
}

Type* TypeCreateAlias(SpanFile* file, Type* baseType, const char* name) {
    if (file->typesCount >= file->typesCapacity) {
        file->types = reallocArena(file->arena, sizeof(Type) * file->typesCapacity, file->types, sizeof(Type) * file->typesCount * 2);
        file->typesCapacity *= 2;
    }
    Type* type = file->types + file->typesCount++;
    *type = *baseType;
    u64 nameLength = strlen(name);
    type->arena = createArena(file->arena, (nameLength + 4) * 2);
    type->name = allocArena(type->arena, nameLength + 1);
    memcpy(type->name, name, nameLength + 1);
    type->alises = NULL;
    type->alisesCount = 0;
    type->baseType = baseType;
    type->origin = to_alias;
    type->templateDefinition = NULL;

    if (baseType->alises == NULL) {
        baseType->alises = allocArena(baseType->arena, sizeof(Type*) * 1);
        baseType->alisesCount = 1;
    } else {
        baseType->alises = reallocArena(baseType->arena, sizeof(Type*) * baseType->alisesCount, baseType->alises, sizeof(Type*) * (baseType->alisesCount + 1));
        baseType->alisesCount++;
    }
    baseType->alises[baseType->alisesCount++] = type;

    return type;
}

Type* TypeCreateDistinct(SpanFile* file, Type* baseType, const char* name) {
    if (file->typesCount >= file->typesCapacity) {
        file->types = reallocArena(file->arena, sizeof(Type) * file->typesCapacity, file->types, sizeof(Type) * file->typesCount * 2);
        file->typesCapacity *= 2;
    }
    Type* type = file->types + file->typesCount++;
    *type = *baseType;
    u64 nameLength = strlen(name);
    type->arena = createArena(file->arena, (nameLength + 4) * 2);
    type->name = allocArena(type->arena, nameLength + 1);
    memcpy(type->name, name, nameLength + 1);
    type->baseType = baseType;
    type->origin = to_distinct;
    type->alises = NULL;
    type->templateDefinition = NULL;
    type->alisesCount = 0;
    return type;
}

void freeType(Type* type) {
    for (u64 i = 0; i < type->alisesCount; i++) {
        Type* alise = type->alises[i];
        alise->baseType = NULL;  // mark this as having no base type will let us know this type is no longer valid
    }
    freeArena(type->arena);
}
