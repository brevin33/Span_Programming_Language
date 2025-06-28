#include "parser.h"
#include <string.h>

Pool projectPool;

void freeProject(Project* project) {
    freeArena(project->arena);
}

Project* getProjectFromId(projectId projectId) {
    return (Project*)poolGetItem(&projectPool, projectId);
}

projectId createProject(const char* directory) {
    projectId id = poolNewItem(&projectPool);
    Project* project = getProjectFromId(id);
    Arena* tempArena = createArena(1024 * 1024);

    project->arena = createArena(1024 * 1024);

    u64 directoryLength = strlen(directory);
    project->directory = arenaAlloc(project->arena, directoryLength + 1);
    memcpy(project->directory, directory, directoryLength + 1);

    // get the name of the project
    const char* dirBack = directory + directoryLength - 1;
    if (dirBack[0] == '\\' || dirBack[0] == '/') {
        dirBack--;
    }
    while (dirBack[0] != '\\' && dirBack[0] != '/') {
        dirBack--;
    }
    const char* name = dirBack + 1;
    u64 nameLength = strlen(name);
    project->name = arenaAlloc(project->arena, nameLength + 1);
    memcpy(project->name, name, nameLength + 1);

    project->sourceCodeIds = arenaAlloc(project->arena, sizeof(sourceCodeId) * 1024);
    project->sourceCodeCount = 0;
    project->sourceCodeCapacity = 1024;

    u64 fileInDirectoryCount = 0;
    char** filesInDirectory = listFilesInDirectory(directory, &fileInDirectoryCount, tempArena);
    for (u64 i = 0; i < fileInDirectoryCount; i++) {
        char* file = filesInDirectory[i];
        u64 fileLength = strlen(file);
        if (fileLength < 5) {
            continue;
        }
        char* last5Chars = file + fileLength - 5;
        if (strcmp(last5Chars, ".span") != 0) {
            continue;
        }

        char* fileWithPath = arenaAlloc(tempArena, fileLength + directoryLength + 2);
        memcpy(fileWithPath, directory, directoryLength);
        if (directory[directoryLength - 1] != '\\' || directory[directoryLength - 1] != '/') {
            fileWithPath[directoryLength] = '/';
            memcpy(fileWithPath + directoryLength + 1, file, fileLength);
            fileWithPath[directoryLength + fileLength + 1] = '\0';
        } else {
            memcpy(fileWithPath + directoryLength, file, fileLength);
            fileWithPath[directoryLength + fileLength] = '\0';
        }

        sourceCodeId sourceCodeId = createSourceCode(fileWithPath);
        if (project->sourceCodeCount >= project->sourceCodeCapacity) {
            project->sourceCodeIds = arenaRealloc(project->arena, project->sourceCodeIds, sizeof(sourceCodeId) * project->sourceCodeCapacity, sizeof(sourceCodeId) * project->sourceCodeCapacity * 2);
            project->sourceCodeCapacity *= 2;
        }
        project->sourceCodeIds[project->sourceCodeCount++] = sourceCodeId;
    }

    for (u64 i = 0; i < project->sourceCodeCount; i++) {
        sourceCodeId sourceCodeId = project->sourceCodeIds[i];
        SourceCode* sourceCode = getSourceCodeFromId(sourceCodeId);
        protoTypeTypes(sourceCodeId, id);
    }

    for (u64 i = 0; i < project->sourceCodeCount; i++) {
        sourceCodeId sourceCodeId = project->sourceCodeIds[i];
        SourceCode* sourceCode = getSourceCodeFromId(sourceCodeId);
        implementTypes(sourceCodeId, id);
    }

    for (u64 i = 0; i < project->sourceCodeCount; i++) {
        sourceCodeId sourceCodeId = project->sourceCodeIds[i];
        SourceCode* sourceCode = getSourceCodeFromId(sourceCodeId);
        protoTypeFunctions(sourceCodeId);
    }

    for (u64 i = 0; i < project->sourceCodeCount; i++) {
        sourceCodeId sourceCodeId = project->sourceCodeIds[i];
        SourceCode* sourceCode = getSourceCodeFromId(sourceCodeId);
        implementSourceCodeFunctions(sourceCodeId);
    }

    freeArena(tempArena);
    return id;
}
