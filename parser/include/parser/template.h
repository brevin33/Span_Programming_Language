#pragma once

#include "parser/map.h"
#include "parser/nice_ints.h"
#include "parser/arena.h"
#include "parser/pool.h"
#include "parser/tokens.h"

typedef u64 typeId;

typedef struct _TemplateDefinition {
    u64 numTypes;
    typeId* interfaces;
    char** templateTypeNames;
} TemplateDefinition;

typedef struct _TemplateInstance {
    typeId* types;
    u64 numTypes;
    char** templateTypeNames;  // might be NULL if just giving the types by order
} TemplateInstance;

TemplateDefinition getTemplateDefinitionFromTokens(Token** tokens, Arena* arena, bool logErrors);

TemplateInstance getTemplateInstanceFromTokens(Token** tokens, Arena* arena, bool logErrors);
