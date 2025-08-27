#pragma once


#include "span_parser/default.h"

u64 countDigits(u64 n);

char* uintToString(u64 number, char* buffer);

char* intToString(i64 number, char* buffer);

char* getDirectoryNameFromPath(char* path, char* buffer);

u64 stringToUint(char* string);

bool stringIsUint(char* string);

bool mkdir(const char* path);

bool rmdir(const char* path);

bool linkExe(char** objFiles, u64 objFilesCount, char* exeName);

int runExe(char* exeName, char* programTextOutputBuffer);

double getTimeSeconds();
