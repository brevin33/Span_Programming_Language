#pragma once


#include "span_parser/default.h"

u64 countDigits(u64 n);

char* uintToString(u64 number, char* buffer);

char* intToString(i64 number, char* buffer);

char* getDirectoryNameFromPath(char* path, char* buffer);

u64 stringToUint(char* string);

bool stringIsUint(char* string);
