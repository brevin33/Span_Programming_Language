#include "span_parser.h"

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#else
    #include <dirent.h>
#endif

char* readFile(Arena arena, char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    u64 fileSize = ftell(file);
    rewind(file);
    char* fileContents = allocArena(arena, fileSize + 1);
    fread(fileContents, 1, fileSize, file);
    fclose(file);
    fileContents[fileSize] = '\0';
    return fileContents;
}

char* getFileType(char* path, char* buffer) {
    char* extension = strrchr(path, '.');
    if (extension == NULL) {
        return NULL;
    }
    u64 extensionLength = strlen(extension);
    memcpy(buffer, extension, extensionLength + 1);
    return buffer;
}

bool deleteDirectory(const char* directory) {
#if defined(_WIN32) || defined(_WIN64)
    WIN32_FIND_DATA findFileData;
    char searchPath[MAX_PATH];
    snprintf(searchPath, MAX_PATH, "%s\\*", directory);
    HANDLE hFind = FindFirstFile(searchPath, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return RemoveDirectory(directory) != 0;
    }

    do {
        if (strcmp(findFileData.cFileName, ".") == 0 || strcmp(findFileData.cFileName, "..") == 0) {
            continue;
        }
        char filePath[MAX_PATH];
        snprintf(filePath, MAX_PATH, "%s\\%s", directory, findFileData.cFileName);

        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!deleteDirectory(filePath)) {
                FindClose(hFind);
                return false;
            }
        } else {
            if (DeleteFile(filePath) == 0) {
                FindClose(hFind);
                return false;
            }
        }
    } while (FindNextFile(hFind, &findFileData));
    FindClose(hFind);
    return RemoveDirectory(directory) != 0;
#else
    DIR* dir = opendir(directory);
    if (!dir) return (rmdir(directory) == 0);

    struct dirent* entry;
    int ret = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        char path[BUFFER_SIZE];
        snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name);

        struct stat statbuf;
        if (stat(path, &statbuf) == 0) {
            if (S_ISDIR(statbuf.st_mode)) {
                if (!deleteDirectory(path)) {
                    ret = -1;
                    break;
                }
            } else {
                if (remove(path) != 0) {
                    ret = -1;
                    break;
                }
            }
        }
    }
    closedir(dir);
    if (ret == 0) ret = rmdir(directory);
    return ret == 0;
#endif
}

char** getFileNamesInDirectory(Arena arena, char* path, u64* outFileCount) {
    u64 filesCapacity = 8;
    u64 fileCount = 0;
    char** files = allocArena(arena, sizeof(char*) * filesCapacity);

#if defined(_WIN32) || defined(_WIN64)
    WIN32_FIND_DATA findFileData;
    char searchPath[MAX_PATH];
    snprintf(searchPath, MAX_PATH, "%s\\*", path);
    HANDLE hFind = FindFirstFile(searchPath, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return NULL;
    }
    for (;;) {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            if (fileCount >= filesCapacity) {
                files = reallocArena(arena, sizeof(char*) * (filesCapacity * 2), files, sizeof(char*) * filesCapacity);
                filesCapacity *= 2;
            }
            size_t len = strlen(findFileData.cFileName) + 1;
            files[fileCount] = (char*)allocArena(arena, len);
            memcpy(files[fileCount], findFileData.cFileName, len);
            fileCount++;
        }
        if (!FindNextFile(hFind, &findFileData)) break;
    }
    FindClose(hFind);
#else
    DIR* dir = opendir(path);
    if (!dir) return NULL;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_DIR) {
            if (fileCount >= filesCapacity) {
                files = reallocArena(arena, sizeof(char*) * (filesCapacity * 2), files, sizeof(char*) * filesCapacity);
                filesCapacity *= 2;
            }
            size_t len = strlen(entry->d_name) + 1;
            files[fileCount] = (char*)allocArena(arena, len);
            memcpy(files[fileCount], entry->d_name, len);
            fileCount++;
        }
    }
    closedir(dir);
#endif
    *outFileCount = fileCount;
    return files;
}

char** getDirectoryNamesInDirectory(Arena arena, char* path, u64* outDirectoryCount) {
    u64 directoryCount = 0;
    u64 directoriesCapacity = 8;
    char** directories = allocArena(arena, sizeof(char*) * directoriesCapacity);

#if defined(_WIN32) || defined(_WIN64)
    WIN32_FIND_DATA findFileData;
    char searchPath[MAX_PATH];
    snprintf(searchPath, MAX_PATH, "%s\\*", path);
    HANDLE hFind = FindFirstFile(searchPath, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return NULL;
    }
    for (;;) {
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && strcmp(findFileData.cFileName, ".") != 0 && strcmp(findFileData.cFileName, "..") != 0) {
            if (directoryCount >= directoriesCapacity) {
                directories = reallocArena(arena, sizeof(char*) * (directoriesCapacity * 2), directories, sizeof(char*) * directoriesCapacity);
                directoriesCapacity *= 2;
            }
            size_t len = strlen(findFileData.cFileName) + 1;
            directories[directoryCount] = (char*)allocArena(arena, len);
            memcpy(directories[directoryCount], findFileData.cFileName, len);
            directoryCount++;
        }
        if (!FindNextFile(hFind, &findFileData)) break;
    }
    FindClose(hFind);
#else
    DIR* dir = opendir(path);
    if (!dir) return NULL;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            if (directoryCount >= directoriesCapacity) {
                directories = reallocArena(arena, sizeof(char*) * (directoriesCapacity * 2), directories, sizeof(char*) * directoriesCapacity);
                directoriesCapacity *= 2;
            }
            size_t len = strlen(entry->d_name) + 1;
            directories[directoryCount] = (char*)allocArena(arena, len);
            memcpy(directories[directoryCount], entry->d_name, len);
            directoryCount++;
        }
    }
    closedir(dir);
#endif
    *outDirectoryCount = directoryCount;
    return directories;
}
