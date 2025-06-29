#pragma once

#include "default.h"
#include "arena.h"

char* readFile(Arena arena, char* path);

char** getFileNamesInDirectory(Arena arena, char* path, u64* outFileCount);

char** getDirectoryNamesInDirectory(Arena arena, char* path, u64* outDirectoryCount);

bool deleteDirectory(const char* directory);
