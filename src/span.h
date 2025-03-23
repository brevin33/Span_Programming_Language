#pragma once
#include "utils.h"
#include "token.h"
#include <string>
#include <iostream>
#include "module.h"
#include "types.h"

// All globlas are stored in a struct
struct CompilerContext {
    vector<Token> tokens;
    bool hadCompileError = false;
    vector<vector<string>> textByFileByLine;
    vector<string> files;
    LLVMContextRef llvmContext;
    LLVMBuilderRef llvmBuilder;
    Module* activeModule;
    Types types;
};
extern CompilerContext context;

void compile(std::string dir);
void logError(const string& err, Token token, bool wholeLine = false);
