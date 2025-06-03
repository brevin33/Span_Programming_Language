#pragma once
#include "parser/nice_ints.h"
#include "parser/project.h"
#include "parser/tokens.h"

void logError(char* error, ...);

void logErrorToken(char* error, Project* project, Token* token, ...);

void logErrorTokens(char* error, Project* project, Token* tokens, u64 numberOfTokens, ...);
