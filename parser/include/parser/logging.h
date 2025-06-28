#pragma once
#include "parser/nice_ints.h"
#include "parser/tokens.h"

#define assert(condition) assertCrash(condition)

extern bool hadError;

void logError(char* error, ...);

void logErrorTokens(Token* token, u64 tokenCount, char* error, ...);

void assertCrash(bool condition);

void printBar();

void crash();
