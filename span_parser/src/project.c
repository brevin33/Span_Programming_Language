#include "span_parser.h"
#include "span_parser/tokens.h"
#include "span_parser/type.h"


void initializeContext(Arena arena) {
    context.arena = arena;
    context.activeProject = NULL;
    context.namespaceCounter = 1;
    context.initialized = true;
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

void SpanFileGetTokensForFile(SpanFile* file, u64 fileIndex) {
    file->tokens = createTokens(file->arena, file->fileContents, fileIndex);

    u64 fileDefinitionStartsCapacity = 4;
    u64 fileDefinitionStartsCount = 0;
    Token** fileDefinitionStarts = allocArena(file->arena, sizeof(Token*) * fileDefinitionStartsCapacity);
}

void SpanFileGetAstForFile(SpanFile* file) {
    file->ast = createAst(file->arena, &file->tokens);
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
    project.namespace = context.namespaceCounter++;

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
        SpanFileGetTokensForFile(&project.files[i], i);
        SpanFileGetAstForFile(&project.files[i]);
    }

    return project;
}

SpanProject createSpanProject(Arena arena, char* path) {
    return createSpanProjectHelper(arena, NULL, path);
}

Namespace _getNamespace(char* name, SpanProject* project) {
    if (strcmp(name, project->name) == 0) {
        return project->namespace;
    }
    for (u64 i = 0; i < project->childCount; i++) {
        Namespace namespace = _getNamespace(name, &project->children[i]);
        if (namespace != NO_NAMESPACE) {
            return namespace;
        }
    }
    return NO_NAMESPACE;
}

Namespace getNamespace(char* name) {
    SpanProject* project = context.activeProject;
    return _getNamespace(name, project);
}

SpanProject* _SpanProjectFromNamespace(Namespace namespace, SpanProject* project) {
    if (project->namespace == namespace) {
        return project;
    }
    for (u64 i = 0; i < project->childCount; i++) {
        SpanProject* child = &project->children[i];
        SpanProject* found = _SpanProjectFromNamespace(namespace, child);
        if (found != NULL) {
            return found;
        }
    }
    massert(false, "namespace not found");
    return NULL;
}

SpanProject* SpanProjectFromNamespace(Namespace namespace) {
    SpanProject* project = context.activeProject;
    return _SpanProjectFromNamespace(namespace, project);
}

SpanFile* SpanFileFromTokenAndNamespace(Token token, Namespace namespace) {
    massert(namespace != NO_NAMESPACE, "namespace must be valid");
    SpanProject* project = SpanProjectFromNamespace(namespace);
    u16 fileIndex = token.file;
    SpanFile* file = &project->files[fileIndex];
    return file;
}
