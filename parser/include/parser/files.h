#pragma once

#include "parser/nice_ints.h"
#include "parser/arena.h"


char* readFile(const char* filePath, Arena* arena);
void writeFile(const char* filePath, const char* content);
char** listFilesInDirectory(const char* directory, u64* fileCount, Arena* arena);
char** listDirectoriesInDirectory(const char* directory, u64* dirCount, Arena* arena);
