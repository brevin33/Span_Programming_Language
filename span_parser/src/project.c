#include "span_parser.h"

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

char* spanFileGetLine(SpanFile* file, u64 line, char* buffer) {
    char* lineStart = file->fileLineStarts[line];
    while (*lineStart != '\n') {
        *buffer = *lineStart;
        buffer++;
        lineStart++;
    }
    *buffer = '\0';
    return buffer;
}

u64 spanFileFindLineFromInternalPointer(SpanFile* file, char* internalPointer) {
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
    file.fileContents = readFile(arena, filePath);
    file.fileLineStarts = getLineStarts(arena, file.fileContents, &file.fileLineStartsCount);
    return file;
}

void getTokensForFile(SpanFile* file, u64 fileIndex) {
    file->tokens = createTokens(file->arena, file->fileContents, fileIndex);
}

SpanProject createSpanProject(Arena arena, char* path) {
    SpanProject project;
    context.activeProject = &project;
    project.arena = createArena(arena, 1024 * 8);

    char** fileNames = getFileNamesInDirectory(arena, path, &project.fileCount);
    project.files = allocArena(arena, sizeof(SpanFile) * project.fileCount);
    for (u64 i = 0; i < project.fileCount; i++) {
        project.files[i] = createSpanFile(arena, fileNames[i], path, i);
        getTokensForFile(&project.files[i], i);
    }

    return project;
}
