#include "parser.h"
#include <string.h>

TemplateDefinition getTemplateDefinitionFromTokens(Token** tokens, Arena* arena, bool logErrors) {
    TemplateDefinition templateDefinition = { 0 };
    Token* token = *tokens;

    if (token->type != tt_lt) {
        return templateDefinition;
    }
    token++;

    u64 numTempalteTypes = 0;
    char* templateTypeNames[256];
    typeId interfaces[256];
    while (token->type != tt_gt) {
        if (token->type != tt_id) {
            if (logErrors) logErrorTokens(token, 1, "Expected template type name");
        }
        char* name = token->str;
        token++;
        if (token->type != tt_colon) {
            if (logErrors) logErrorTokens(token, 1, "Expected colon");
            return templateDefinition;
        }
        token++;
        typeId tid = getTypeIdFromTokens(&token);
        if (tid == BAD_ID) {
            if (logErrors) logErrorTokens(token, 1, "Expected interface type");
            return templateDefinition;
        }
        Type* type = getTypeFromId(tid);
        if (type->kind != tk_interface) {
            if (logErrors) logErrorTokens(token, 1, "Expected interface type");
            return templateDefinition;
        }
        if (numTempalteTypes >= 256) {
            if (logErrors) logErrorTokens(token, 1, "Too many template types");
            return templateDefinition;
        }
        interfaces[numTempalteTypes] = tid;
        templateTypeNames[numTempalteTypes] = name;
        numTempalteTypes++;
    }
    if (numTempalteTypes == 0) {
        if (logErrors) logErrorTokens(token, 1, "Expected at least one template type");
        return templateDefinition;
    }

    templateDefinition.numTypes = numTempalteTypes;
    templateDefinition.interfaces = arenaAlloc(arena, sizeof(typeId) * numTempalteTypes);
    memcpy(templateDefinition.interfaces, interfaces, sizeof(typeId) * numTempalteTypes);
    templateDefinition.templateTypeNames = arenaAlloc(arena, sizeof(char*) * numTempalteTypes);
    for (u64 i = 0; i < numTempalteTypes; i++) {
        u64 nameSize = strlen(templateTypeNames[i]) + 1;
        templateDefinition.templateTypeNames[i] = arenaAlloc(arena, nameSize);
        memcpy(templateDefinition.templateTypeNames[i], templateTypeNames[i], nameSize);
    }

    *tokens = token;
    return templateDefinition;
}


TemplateInstance getTemplateInstanceFromTokens(Token** tokens, Arena* arena, bool logErrors) {
    TemplateInstance templateInstance = { 0 };
    Token* token = *tokens;

    if (token->type != tt_lt) {
        return templateInstance;
    }
    token++;

    u64 numTemplateTypes = 0;
    char* templateTypeNames[256];
    typeId types[256];
    bool usingNames = false;
    while (token->type != tt_gt) {
        if (usingNames) {
            if (token->type != tt_id) {
                if (logErrors) logErrorTokens(token, 1, "Expected template type name");
                return templateInstance;
            }
            char* name = token->str;
            token++;
            if (token->type != tt_colon) {
                if (logErrors) logErrorTokens(token, 1, "Expected colon");
                return templateInstance;
            }
            token++;
            typeId tid = getTypeIdFromTokens(&token);
            if (tid == BAD_ID) {
                if (logErrors) logErrorTokens(token, 1, "Expected type");
                return templateInstance;
            }
            types[numTemplateTypes] = tid;
            templateTypeNames[numTemplateTypes] = name;
            numTemplateTypes++;
            continue;
        }
        if (token->type == tt_id && token[1].type == tt_colon) {
            if (numTemplateTypes != 0 && usingNames == false) {
                if (logErrors) logErrorTokens(token, 1, "Templates must all use names or none");
                return templateInstance;
            }
            usingNames = true;
            continue;
        }
        typeId tid = getTypeIdFromTokens(&token);
        if (tid == BAD_ID) {
            if (logErrors) logErrorTokens(token, 1, "Expected type");
            return templateInstance;
        }
        if (numTemplateTypes >= 256) {
            if (logErrors) logErrorTokens(token, 1, "Too many template types");
            return templateInstance;
        }
        types[numTemplateTypes] = tid;
        numTemplateTypes++;
    }

    templateInstance.numTypes = numTemplateTypes;
    templateInstance.types = arenaAlloc(arena, sizeof(typeId) * numTemplateTypes);
    memcpy(templateInstance.types, types, sizeof(typeId) * numTemplateTypes);
    if (usingNames) {
        templateInstance.templateTypeNames = arenaAlloc(arena, sizeof(char*) * numTemplateTypes);
        for (u64 i = 0; i < numTemplateTypes; i++) {
            u64 nameSize = strlen(templateTypeNames[i]) + 1;
            templateInstance.templateTypeNames[i] = arenaAlloc(arena, nameSize);
            memcpy(templateInstance.templateTypeNames[i], templateTypeNames[i], nameSize);
        }
    } else {
        templateInstance.templateTypeNames = NULL;
    }

    *tokens = token;
    return templateInstance;
}
