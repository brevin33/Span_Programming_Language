#include "parser/files.h"
#include "parser/logging.h"
#include "parser/arena.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#else
    #include <dirent.h>
#endif

char* readFile(const char* filePath, Arena* arena) {
    FILE* file = fopen(filePath, "rb");
    if (!file) {
        logError("Failed to open file for reading");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* content = (char*)arenaAlloc(arena, fileSize + 1);
    if (!content) {
        fclose(file);
        logError("Failed to allocate memory for file content");
        return NULL;
    }

    fread(content, 1, fileSize, file);
    content[fileSize] = '\0';  // Null-terminate the string
    fclose(file);

    return content;
}

void writeFile(const char* filePath, const char* content) {
    FILE* file = fopen(filePath, "w");
    if (!file) {
        logError("Failed to open file for writing");
        return;
    }

    size_t contentLength = strlen(content);
    size_t written = fwrite(content, 1, contentLength, file);
    if (written < contentLength) {
        logError("Failed to write complete content to file");
    }

    fclose(file);
}

char** listFilesInDirectory(const char* directory, u64* fileCount, Arena* arena) {
    char** files = NULL;
    *fileCount = 0;

#if defined(_WIN32) || defined(_WIN64)
    WIN32_FIND_DATA findFileData;
    char searchPath[MAX_PATH];
    snprintf(searchPath, MAX_PATH, "%s\\*", directory);
    HANDLE hFind = FindFirstFile(searchPath, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return NULL;
    }
    for (;;) {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            files = (char**)arenaRealloc(arena, files, sizeof(char*) * (*fileCount), sizeof(char*) * (*fileCount + 1));
            size_t len = strlen(findFileData.cFileName) + 1;
            files[*fileCount] = (char*)arenaAlloc(arena, len);
            memcpy(files[*fileCount], findFileData.cFileName, len);
            (*fileCount)++;
        }
        if (!FindNextFile(hFind, &findFileData)) break;
    }
    FindClose(hFind);
#else
    DIR* dir = opendir(directory);
    if (!dir) return NULL;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_DIR) {
            files = (char**)arenaRealloc(arena, files, sizeof(char*) * (*fileCount), sizeof(char*) * (*fileCount + 1));
            size_t len = strlen(entry->d_name) + 1;
            files[*fileCount] = (char*)arenaAlloc(arena, len);
            memcpy(files[*fileCount], entry->d_name, len);
            (*fileCount)++;
        }
    }
    closedir(dir);
#endif
    return files;
}

char** listDirectoriesInDirectory(const char* directory, u64* dirCount, Arena* arena) {
    char** dirs = NULL;
    *dirCount = 0;

#if defined(_WIN32) || defined(_WIN64)
    WIN32_FIND_DATA findFileData;
    char searchPath[MAX_PATH];
    snprintf(searchPath, MAX_PATH, "%s\\*", directory);
    HANDLE hFind = FindFirstFile(searchPath, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return NULL;
    }
    for (;;) {
        if ((findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && strcmp(findFileData.cFileName, ".") != 0 && strcmp(findFileData.cFileName, "..") != 0) {
            dirs = (char**)arenaRealloc(arena, dirs, sizeof(char*) * (*dirCount), sizeof(char*) * (*dirCount + 1));
            size_t len = strlen(findFileData.cFileName) + 1;
            dirs[*dirCount] = (char*)arenaAlloc(arena, len);
            memcpy(dirs[*dirCount], findFileData.cFileName, len);
            (*dirCount)++;
        }
        if (!FindNextFile(hFind, &findFileData)) break;
    }
    FindClose(hFind);
#else
    DIR* dir = opendir(directory);
    if (!dir) return NULL;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            dirs = (char**)arenaRealloc(arena, dirs, sizeof(char*) * (*dirCount), sizeof(char*) * (*dirCount + 1));
            size_t len = strlen(entry->d_name) + 1;
            dirs[*dirCount] = (char*)arenaAlloc(arena, len);
            memcpy(dirs[*dirCount], entry->d_name, len);
            (*dirCount)++;
        }
    }
    closedir(dir);
#endif
    return dirs;
}
