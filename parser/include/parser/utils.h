#pragma once


#include "parser/arena.h"
#include "parser/nice_ints.h"



char* addStrings(const char* a, const char* b, Arena* arena);

char* subStrings(const char* a, const char* b, Arena* arena);

char* mulStrings(const char* a, const char* b, Arena* arena);

char* divStrings(const char* a, const char* b, Arena* arena);

char* modStrings(const char* a, const char* b, Arena* arena);

int compareStrings(const char* a, const char* b);

char* twoToThePowerOf(u64 numBits, Arena* arena);
