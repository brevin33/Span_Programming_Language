#pragma once
#include "parser/nice_ints.h"
#include "parser/tokens.h"

void logError(char* error, ...);

void logErrorTokens(Token* token, u64 tokenCount, char* error, ...);
