#pragma once
#include "parser/nice_ints.h"
#include <string>
#include <iostream>
#include "tokens.h"

void logError(std::string error);
void logError(std::string error, Token token, Tokens& tokens);
void drawLine();
