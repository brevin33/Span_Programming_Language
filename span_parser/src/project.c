#include "span_parser.h"


void initializeContext(Arena arena) {
    context.arena = arena;
    context.activeProject = NULL;

    context.baseTypes = allocArena(arena, sizeof(Type) * 64);
    context.baseTypesCount = 0;
    context.baseTypesCapacity = 64;

    Type* intType = getNumberType(64, tk_int);
    TypeCreateAliasGlobal(intType, "int");
    Type* uintType = getNumberType(64, tk_uint);
    TypeCreateAliasGlobal(uintType, "uint");
    Type* floatType = getNumberType(32, tk_float);
    TypeCreateAliasGlobal(floatType, "float");
    Type* doubleType = getNumberType(64, tk_float);
    TypeCreateAliasGlobal(doubleType, "double");
    Type* boolType = getNumberType(1, tk_bool);
    TypeCreateAliasGlobal(boolType, "bool");

    context.initialized = true;
}

TemplateDefinition* createTemplateDefinitionFromTokens(Arena arena, Token** tokens) {
    Token* token = *tokens;
    if (token->type != tt_lt) {
        return NULL;
    }
    token++;
    char** names = allocArena(arena, sizeof(char*) * 8);
    Type** interfaces = allocArena(arena, sizeof(Type*) * 8);
    u64 templateCount = 0;
    u64 templateCapacity = 8;
    while (true) {
        if (token->type == tt_gt) {
            break;
        }
        if (token->type != tt_id) {
            return NULL;
        }
        if (templateCount >= templateCapacity) {
            names = reallocArena(arena, sizeof(char*) * templateCapacity, names, sizeof(char*) * (templateCapacity * 2));
            interfaces = reallocArena(arena, sizeof(Type*) * templateCapacity, interfaces, sizeof(Type*) * (templateCapacity * 2));
            templateCapacity *= 2;
        }
        char buffer[4096];
        char* name = tokenGetString(*token, buffer);
        u64 nameLength = strlen(name);
        names[templateCount] = allocArena(arena, nameLength + 1);
        memcpy(names[templateCount], name, nameLength + 1);
        token++;
        if (token->type == tt_colon) {
            token++;
            Type* interface = TypeFromTokens(NULL, &token, false);
            if (interface == NULL) {
                return NULL;
            }
            if (interface->kind != tk_interface) {
                return NULL;
            }
            interfaces[templateCount] = interface;
        } else {
            interfaces[templateCount] = NULL;
        }
        templateCount++;
    }
    if (templateCount == 0) {
        return NULL;
    }

    TemplateDefinition* templateDefinition = allocArena(arena, sizeof(TemplateDefinition));
    templateDefinition->names = names;
    templateDefinition->interfaces = interfaces;
    templateDefinition->templateCount = templateCount;
    *tokens = token;
    return templateDefinition;
}

TemplateInstance* createTemplateInstanceCreateFromTokens(Arena arena, Token** tokens) {
    Token* token = *tokens;
    if (token->type != tt_lt) {
        return NULL;
    }
    token++;
    Type** types = allocArena(arena, sizeof(Type*) * 8);
    u64 templateCount = 0;
    u64 templateCapacity = 8;
    while (true) {
        if (token->type == tt_gt) {
            break;
        }
        Type* type = TypeFromTokens(NULL, &token, false);
        if (type == NULL) {
            return NULL;
        }
        if (templateCount >= templateCapacity) {
            types = reallocArena(arena, sizeof(Type*) * templateCapacity, types, sizeof(Type*) * (templateCapacity * 2));
            templateCapacity *= 2;
        }
        types[templateCount++] = type;
    }
    if (templateCount == 0) {
        return NULL;
    }

    TemplateInstance* templateInstance = allocArena(arena, sizeof(TemplateInstance));
    templateInstance->types = types;
    templateInstance->templateCount = templateCount;
    *tokens = token;
    return templateInstance;
}


char** getLineStarts(Arena arena, char* fileContents, u64* outLineStartsCount) {
    u64 lineStartsCount = 0;
    u64 lineStartsCapacity = 8;
    char** lineStarts = allocArena(arena, sizeof(char*) * lineStartsCapacity);
    lineStarts[lineStartsCount++] = fileContents;
    while (*fileContents != '\0') {
        if (*fileContents == '\n') {
            lineStarts[lineStartsCount++] = fileContents + 1;
        }
        fileContents++;
    }
    *outLineStartsCount = lineStartsCount;
    return lineStarts;
}

char* SpanFileGetLine(SpanFile* file, u64 line, char* buffer) {
    char* lineStart = file->fileLineStarts[line];
    while (*lineStart != '\n') {
        *buffer = *lineStart;
        buffer++;
        lineStart++;
    }
    *buffer = '\0';
    return buffer;
}

u64 SpanFileFindLineFromInternalPointer(SpanFile* file, char* internalPointer) {
    u64 line = 0;
    for (u64 i = 0; i < file->fileLineStartsCount; i++) {
        if (file->fileLineStarts[i] > internalPointer) {
            return line;
        }
        line++;
    }
    massert(false, "didn't get passed an internal pointer");
    return line;
}

SpanFile createSpanFile(Arena arena, char* fileName, char* directory, u64 fileIndex) {
    SpanFile file;
    file.arena = createArena(arena, 1024);
    file.fileName = fileName;
    char filePath[4096];
    sprintf(filePath, "%s/%s", directory, fileName);
    file.fileContents = readFile(file.arena, filePath);
    file.fileLineStarts = getLineStarts(file.arena, file.fileContents, &file.fileLineStartsCount);
    file.types = allocArena(file.arena, sizeof(Type) * 8);
    file.typesCount = 0;
    file.typesCapacity = 8;
    return file;
}

char* getNameSpaceFromTokens(Token** tokens, char* buffer) {
    Token* token = *tokens;
    if (token->type != tt_id && token[1].type != tt_colon_colon) {
        return NULL;
    }
    char* namespace = tokenGetString(*token, buffer);
    token += 2;
    *tokens = token;
    return namespace;
}

void getTokensForFile(SpanFile* file, u64 fileIndex) {
    file->tokens = createTokens(file->arena, file->fileContents, fileIndex);
}

SpanProject createSpanProjectHelper(Arena arena, SpanProject* parent, char* path) {
    if (!context.initialized) {
        initializeContext(arena);
    }
    SpanProject project;
    context.activeProject = &project;
    project.arena = createArena(arena, 1024 * 8);
    project.name = allocArena(project.arena, 4096);
    project.name = getDirectoryNameFromPath(path, project.name);
    project.parent = parent;

    u64 directoryCount;
    char** directories = getDirectoryNamesInDirectory(project.arena, path, &directoryCount);
    project.childCount = 0;
    project.childCapacity = directoryCount;
    project.children = allocArena(project.arena, sizeof(SpanProject) * directoryCount);
    for (u64 i = 0; i < directoryCount; i++) {
        char* directory = directories[i];
        char directoryPath[4096];
        sprintf(directoryPath, "%s/%s", path, directory);
        SpanProject child = createSpanProjectHelper(project.arena, &project, directoryPath);
        project.children[project.childCount++] = child;
    }

    char** fileNames = getFileNamesInDirectory(project.arena, path, &project.fileCount);
    project.files = allocArena(project.arena, sizeof(SpanFile) * project.fileCount);
    for (u64 i = 0; i < project.fileCount; i++) {
        project.files[i] = createSpanFile(project.arena, fileNames[i], path, i);
        getTokensForFile(&project.files[i], i);
    }

    return project;
}

SpanProject createSpanProject(Arena arena, char* path) {
    return createSpanProjectHelper(arena, NULL, path);
}
