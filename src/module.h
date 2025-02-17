#pragma once
#include "utils.h"
#include "token.h"
#include "function.h"

class Function;

class Module {
public:
    Module(const string& dir);

    ~Module();

    void loadTokens();

    void findStarts();

    void setupTypesAndFunctions();

    void logError(const string& err, Token* token = nullptr, bool wholeLine = false);

    void printResult();

public:
    LLVMModuleRef llvmModule;

private:
    string dir;
    LLVMTargetDataRef dataLayout;
    vector<string> files;
    vector<vector<string>> textByFileByLine;
    vector<TokenPositon> functionStarts;
    vector<Module*> moduleDeps;
    Tokens tokens;
    bool hadError = false;

private:
    optional<Type> typeFromTokens();

    Function* prototypeFunction(TokenPositon start);

    void implementFunction(TokenPositon start, Function* func);

    bool looksLikeType();

    bool looksLikeFunction();
};